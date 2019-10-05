#pragma once

#include "option.h"
#include "status.h"
#include "env.h"

class LogWriter {
public:
	// Create a writer that will append data to "*dest".
	// "*dest" must be initially empty.
	// "*dest" must remain live while this Writer is in use.
	explicit LogWriter(WritableFile* dest);

	// Create a writer that will append data to "*dest".
	// "*dest" must have initial length "dest_length".
	// "*dest" must remain live while this Writer is in use.
	LogWriter(WritableFile* dest, uint64_t destlength);

	Status AddRecord(const std::string_view& slice);

private:
	WritableFile* dest;
	int blockoffset;       // Current offset in block
	// crc32c values for all supported record types.  These are
	// pre-computed to reduce the overhead of computing the crc of the
	// record type stored in the header.
	uint32_t typecrc[kMaxRecordType + 1];

	Status EmitPhysicalRecord(RecordType type, const char* ptr, size_t length);

	// No copying allowed
	LogWriter(const LogWriter&);

	void operator=(const LogWriter&);
};



