#include "merger.h"
#include "iterator.h"

class MergingIterator : public Iterator {
public:
	MergingIterator(const Comparator* comparator, std::vector<std::shared_ptr<Iterator>>& child, int n)
		: comparator(comparator),
		n(n),
		current(nullptr),
		direction(kForward) {
		for (int i = 0; i< child.size(); i++) {
			std::shared_ptr<IteratorWrapper> iterWrap(new IteratorWrapper(child[i]));
			children.push_back(iterWrap);
		}
	}

	virtual ~MergingIterator() {

	}

	virtual bool Valid() const {
		return (current != nullptr);
	}

	virtual void SeekToFirst() {
		for (int i = 0; i< n; i++) {
			children[i]->SeekToFirst();
		}

		findSmallest();
		direction = kForward;
	}

	virtual void SeekToLast() {
		for (int i = 0; i< n; i++) {
			children[i]->SeekToLast();
		}

		findLargest();
		direction = kReverse;
	}

	virtual void Seek(const std::string_view& target) {
		for (int i = 0; i< n; i++) {
			children[i]->Seek(target);
		}

		findSmallest();
		direction = kForward;
	}

	virtual void Next() {
		assert(Valid());

		// Ensure that all children are positioned after key().
		// If we are moving in the forward direction, it is already
		// true for all of the non-current_ children since current_ is
		// the smallest child and key() == current_->key().  Otherwise,
		// we explicitly position the non-current_ children.
		if (direction != kForward) {
			for (int i = 0; i< n; i++) {
				auto child = children[i];
				if (child != current) {
					child->Seek(key());
					if (child->Valid() &&
						comparator->Compare(key(), child->key()) == 0) {
						child->Next();
					}
				}
			}
			direction = kForward;
		}

		current->Next();
		findSmallest();
	}

	virtual void Prev() {
		assert(Valid());

		// Ensure that all children are positioned before key().
		// If we are moving in the reverse direction, it is already
		// true for all of the non-current_ children since current_ is
		// the largest child and key() == current_->key().  Otherwise,
		// we explicitly position the non-current_ children.
		if (direction != kReverse) {
			for (int i = 0; i< n; i++) {
				auto child = children[i];
				if (child != current) {
					child->Seek(key());
					if (child->Valid()) {
						// Child is at first entry >= key().  Step back one to be< key()
						child->Prev();
					}
					else {
						// Child has no entries >= key().  Position at last entry.
						child->SeekToLast();
					}
				}
			}
			direction = kReverse;
		}

		current->Prev();
		findLargest();
	}

	virtual std::string_view key() const {
		assert(Valid());
		return current->key();
	}

	virtual std::string_view value() const {
		assert(Valid());
		return current->value();
	}

	virtual Status status() const {
		Status status;
		for (int i = 0; i< n; i++) {
			status = children[i]->status();
			if (!status.ok()) {
				break;
			}
		}
		return status;
	}

	virtual void RegisterCleanup(const std::any& arg) {
		clearnups.push_back(arg);
	}

private:
	void findSmallest();

	void findLargest();

	// We might want to use a heap in case there are lots of children.
	// For now we use a simple array since we expect a very small number
	// of children in leveldb.
	const Comparator* comparator;
	std::vector<std::shared_ptr<IteratorWrapper>> children;
	std::shared_ptr<IteratorWrapper> current;

	int n;
	// Which direction is the iterator moving?
	enum Direction {
		kForward,
		kReverse
	};
	Direction direction;
	std::list<std::any> clearnups;
};

void MergingIterator::findSmallest() {
	std::shared_ptr<IteratorWrapper> smallest = nullptr;
	for (int i = 0; i< n; i++) {
		std::shared_ptr<IteratorWrapper> child = children[i];
		if (child->Valid()) {
			if (smallest == nullptr) {
				smallest = child;
			}
			else if (comparator->Compare(child->key(), smallest->key())< 0) {
				smallest = child;
			}
		}
	}
	current = smallest;
}

void MergingIterator::findLargest() {
	std::shared_ptr<IteratorWrapper> largest = nullptr;
	for (int i = n - 1; i >= 0; i--) {
		std::shared_ptr<IteratorWrapper> child = children[i];
		if (child->Valid()) {
			if (largest == nullptr) {
				largest = child;
			}
			else if (comparator->Compare(child->key(), largest->key()) > 0) {
				largest = child;
			}
		}
	}
	current = largest;
}

std::shared_ptr<Iterator> NewMergingIterator(
	const Comparator* cmp, std::vector<std::shared_ptr<Iterator>>& list, int n) {
	assert(n >= 0);
	if (n == 0) {
		return NewEmptyIterator();
	}
	else if (n == 1) {
		return list[0];
	}
	else {
		std::shared_ptr<MergingIterator> iter(new MergingIterator(cmp, list, n));
		return iter;
	}
}
