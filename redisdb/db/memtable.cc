#include "memtable.h"
#include "coding.h"

MemTable::MemTable(const InternalKeyComparator& comparator)
	: memoryusage(0),
	kcmp(comparator),
	table(comparator) {

}

MemTable::~MemTable() {
	ClearTable();
}

bool MemTable::Get(const LookupKey& key, std::string* value, Status* s) {
	std::string_view memkey = key.MemtableKey();
	Table::Iterator iter(&table);
	iter.Seek(memkey.data());
	if (iter.Valid()) {
		// entry format is:
		// klength  varint32
		// userkey  char[klength]
		// tag      uint64
		// vlength  varint32
		// value    char[vlength]
		// Check that it belongs to same user key.  We do not check the
		// sequence number since the Seek() call above should have skipped
		// all entries with overly large sequence numbers.

		const char* entry = iter.key();
		uint32_t keylength;
		const char* keyptr = GetVarint32Ptr(entry, entry + 5, &keylength);

		if (kcmp.icmp.GetComparator()->Compare(std::string_view(keyptr, keylength - 8),
			key.UserKey()) == 0) {
			const uint64_t tag = DecodeFixed64(keyptr + keylength - 8);
			switch (static_cast<ValueType>(tag & 0xff)) {
			case kTypeValue: {
				std::string_view v = GetLengthPrefixedSlice(keyptr + keylength);
				value->assign(v.data(), v.size());
				return true;
			}

			case kTypeDeletion:
				*s = Status::NotFound(std::string_view());
				return true;
			}
		}
	}
	return false;
}

void MemTable::Add(uint64_t seq, ValueType type, const std::string_view& key,
	const std::string_view& value) {
	// Format of an entry is concatenation of:
	//  key_size     : varint32 of internal_key.size()
	//  key bytes   : char[internal_key.size()]
	//  value_size   : varint32 of value.size()
	//  value bytes : char[value.size()]

	size_t keysize = key.size();
	size_t valsize = value.size();
	size_t internalkeysize = keysize + 8;

	const size_t encodedlen =
		VarintLength(internalkeysize) + internalkeysize +
		VarintLength(valsize) + valsize;

	char* buf = new char[encodedlen];
	char* p = EncodeVarint32(buf, internalkeysize);
	memcpy(p, key.data(), keysize);
	p += keysize;
	EncodeFixed64(p, (seq << 8) | type);
	p += 8;
	p = EncodeVarint32(p, valsize);
	memcpy(p, value.data(), valsize);
	assert(p + valsize == buf + encodedlen);
	table.Insert(buf);
	memoryusage += encodedlen;
}

int MemTable::KeyComparator::operator()(const char* aptr, const char* bptr) const {
	// Internal keys are encoded as length-prefixed strings.
	std::string_view a = GetLengthPrefixedSlice(aptr);
	std::string_view b = GetLengthPrefixedSlice(bptr);
	return icmp.Compare(a, b);
}

void MemTable::ClearTable() {
	SkipList<const char*, KeyComparator>::Iterator iter(&table);
	for (iter.SeekToFirst(); iter.Valid(); iter.Next()) {
		delete[] iter.key();
	}
	memoryusage = 0;
}

// Encode a suitable internal key target for "target" and return it.
// Uses *scratch as scratch space, and the returned pointer will point
// into this scratch space.
static const char* EncodeKey(std::string* scratch, const std::string_view& target) {
	scratch->clear();
	PutVarint32(scratch, target.size());
	scratch->append(target.data(), target.size());
	return scratch->data();
}

class MemTableIterator : public Iterator {
public:
	explicit MemTableIterator(MemTable::Table* table) : iter(table) { }

	virtual bool Valid() const { return iter.Valid(); }

	virtual void Seek(const std::string_view& k) { iter.Seek(EncodeKey(&tmp, k)); }

	virtual void SeekToFirst() { iter.SeekToFirst(); }

	virtual void SeekToLast() { iter.SeekToLast(); }

	virtual void Next() { iter.Next(); }

	virtual void Prev() { iter.Prev(); }

	virtual std::string_view key() const { return GetLengthPrefixedSlice(iter.key()); }

	virtual std::string_view value() const {
		std::string_view keyview = GetLengthPrefixedSlice(iter.key());
		return GetLengthPrefixedSlice(keyview.data() + keyview.size());
	}

	virtual Status status() const { return Status::OK(); }

private:
	MemTable::Table::Iterator iter;
	std::string tmp;       // For passing to EncodeKey

	// No copying allowed
	MemTableIterator(const MemTableIterator&);
	void operator=(const MemTableIterator&);
};

std::shared_ptr<Iterator> MemTable::NewIterator() {
	std::shared_ptr<Iterator> it(new MemTableIterator(&table));
	return it;
}


