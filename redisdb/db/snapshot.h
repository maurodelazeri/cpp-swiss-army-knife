#pragma once
#include <list>
#include "dbformat.h"

class SnapshotList;

class Snapshot {
public:
public:
	Snapshot(uint64_t sequencenumber)
		: sequencenumber(sequencenumber) {
	}

	uint64_t GetSequenceNumber() const {
		return sequencenumber;
	}
private:
	const uint64_t sequencenumber;
};

class SnapshotList {
public:
	bool empty() const {
		return lists.empty();
	}

	std::shared_ptr<Snapshot> oldest() const {
		assert(!lists.empty());
		return lists.front();
	}

	std::shared_ptr<Snapshot> newest() const {
		assert(!lists.empty());
		return lists.back();
	}

	// Creates a SnapshotImpl and appends it to the end of the list.
	const std::shared_ptr<Snapshot> NewSnapshot(uint64_t sequencenumber) {
		assert(empty() || newest()->GetSequenceNumber()<= sequencenumber);
		std::shared_ptr<Snapshot> snapshot(new Snapshot(sequencenumber));
		return snapshot;
	}

	void DeleteSnapshot(const std::shared_ptr<Snapshot>& shapshot) {
		for (auto it = lists.begin(); it != lists.end();) {
			if ((*it)->GetSequenceNumber() == shapshot->GetSequenceNumber()) {
				lists.erase(it++);
				break;
			}
			else {
				it++;
			}
		}
	}
private:
	std::list<std::shared_ptr<Snapshot>> lists;
};
