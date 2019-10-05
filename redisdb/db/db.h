#pragma once
#include <deque>
#include <set>
#include <memory>
#include <mutex>
#include <condition_variable>

#include "logwriter.h"
#include "versionedit.h"
#include "versionset.h"
#include "option.h"
#include "status.h"
#include "writebatch.h"
#include "env.h"
#include "tablecache.h"
#include "snapshot.h"

// A range of keys
struct Range {
	std::string_view start;          // Included in the range
	std::string_view limit;          // Not included in the range

	Range() {}

	Range(const std::string_view& s, const std::string_view& l) : start(s), limit(l) {}
};

class DB {
public:
	DB(const Options& options, const std::string& dbname);

	~DB();

	Status Open();

	Status NewDB();

	// Set the database entry for "key" to "value".  Returns OK on success,
	// and a non-OK status on error.
	// Note: consider setting options.sync = true.
	Status Put(const WriteOptions&, const std::string_view& key, const std::string_view& value);

	// Remove the database entry (if any) for "key".  Returns OK on
	// success, and a non-OK status on error.  It is not an error if "key"
	// did not exist in the database.
	// Note: consider setting options.sync = true.
	Status Delete(const WriteOptions&, const std::string_view& key);

	// Apply the specified updates to the database.
	// Returns OK on success, non-OK on failure.
	// Note: consider setting options.sync = true.
	Status Write(const WriteOptions& options, WriteBatch* updates);

	// If the database Contains an entry for "key" store the
	// corresponding value in *value and return OK.
	//
	// If there is no entry for "key" leave *value unchanged and return
	// a status for which Status::IsNotFound() returns true.
	//
	// May return some other Status on an error.
	Status Get(const ReadOptions& options, const std::string_view& key, std::string* value);

	Status DestroyDB(const std::string& dbname, const Options& options);

	void DeleteObsoleteFiles();

	// Recover the descriptor from persistent storage.  May do a significant
	// amount of work to Recover recently logged updates.  Any changes to
	// be made to the descriptor are added to *edit.
	Status Recover(VersionEdit* edit, bool* savemanifest);

	Status RecoverLogFile(uint64_t lognumber, bool lastLog,
		bool* savemanifest, VersionEdit* edit, uint64_t* maxsequence);

	Status WriteLevel0Table(const std::shared_ptr<MemTable>& mem, VersionEdit* edit, Version* base);

	Status BuildTable(FileMetaData* meta, const std::shared_ptr<Iterator>& iter);

	void MaybeScheduleCompaction();

	void BackgroundCompaction();

	void CompactMemTable();

	std::shared_ptr<Iterator> NewInternalIterator(const ReadOptions& options,
		uint64_t* latestsnapshot, uint32_t* seed);

	// Return a heap-allocated iterator over the Contents of the database.
	// The result of NewIterator() is initially invalid (caller must
	// call one of the Seek methods on the iterator before using it).
	//
	// Caller should delete the iterator when it is no longer needed.
	// The returned iterator should be deleted before this db is deleted.
	std::shared_ptr<Iterator> NewIterator(const ReadOptions& options);

	const std::shared_ptr<Snapshot> GetSnapshot();

	void ReleaseSnapshot(const std::shared_ptr<Snapshot>& shapsnot);

	Options SanitizeOptions(const std::string& dbname,
		const InternalKeyComparator* icmp,
		const Options& src);

	// DB implementations can export properties about their state
	// via this method.  If "property" is a Valid property understood by this
	// DB implementation, fills "*value" with its current value and returns
	// true.  Otherwise returns false.
	//
	//
	// Valid property names include:
	//
	//  "leveldb.num-files-at-level<N>" - return the number of files at level<N>,
	//     where<N> is an ASCII representation of a level number (e.g. "0").
	//  "leveldb.stats" - returns a multi-line string that describes statistics
	//     about the internal operation of the DB.
	//  "leveldb.sstables" - returns a multi-line string that describes all
	//     of the sstables that make up the db Contents.
	//  "leveldb.approximate-memory-usage" - returns the approximate number of
	//     bytes of memory in use by the DB.
	bool GetProperty(const std::string_view& property, std::string* value);

	// For each i in [0,n-1], store in "sizes[i]", the approximate
	// file system space used by keys in "[range[i].start .. range[i].limit)".
	//
	// Note that the returned sizes measure file system space usage, so
	// if the user data compresses by a factor of ten, the returned
	// sizes will be one-tenth the size of the corresponding user data size.
	//
	// The results may not include the sizes of recently written data.
	void GetApproximateSizes(const Range* range, int n, uint64_t* sizes);

	void CompactRange(const std::string_view* begin, const std::string_view* end);

	// Force current memtable Contents to be compacted.
	Status TESTCompactMemTable();

	// Compact any files in the named level that overlap [*begin,*end]
	void TESTCompactRange(int level, const std::string_view* begin, const std::string_view* end);

	// Return an internal iterator over the current state of the database.
	// The keys of this iterator are internal keys (see format.h).
	// The returned iterator should be deleted when no longer needed.
	std::shared_ptr<Iterator> TESTNewInternalIterator();

	// Return the maximum overlapping data (in bytes) at Next level for any
	// file at a level >= 1.
	int64_t TESTMaxNextLevelOverlappingBytes();

	void BackgroundCallback();

	void MaybeIgnoreError(Status* s) const;

	void RecordBackgroundError(const Status& s);

private:
	struct Writer;
	struct CompactionState;

	// No copying allowed
	DB(const DB&);

	void operator=(const DB&);

	Status MakeRoomForWrite(std::unique_lock<std::mutex>& lk, bool force /* compact even if there is room? */);

	WriteBatch* BuildBatchGroup(Writer** lastwriter);

	Status DoCompactionWork(CompactionState* compact);

	Status FinishCompactionOutputFile(CompactionState* compact,
		const std::shared_ptr<Iterator>& input);

	Status OpenCompactionOutputFile(CompactionState* compact);

	Status InstallCompactionResults(CompactionState* compact);

	void CleanupCompaction(CompactionState* compact);

	const Comparator* GetComparator() const {
		return internalcomparator.GetComparator();
	}

	std::atomic<bool> shuttingdown;
	std::atomic<bool> hasimm;         // So bg thread can detect non-null imm_
	// Has a background compaction been scheduled or is running?
	bool bgcompactionscheduled;

	// Queue of writers.
	std::deque<Writer*> writers;
	std::shared_ptr<WriteBatch> tmpbatch;
	std::shared_ptr<MemTable> mem;
	std::shared_ptr<MemTable> imm;
	std::shared_ptr<VersionSet> versions;
	std::shared_ptr<LogWriter> log;
	std::shared_ptr<WritableFile> logfile;
	std::shared_ptr<TableCache> tablecache;
	std::shared_ptr<SnapshotList> snapshots;

	// Lock over the persistent DB state.  Non-null iff successfully acquired.
	std::shared_ptr<FileLock> dblock;

	const Options options;
	uint64_t logfilenumber;
	const std::string dbname;
	uint32_t seed;

	// Set of table files to protect from deletion because they are
	// part of ongoing compactions.
	std::set<uint64_t> pendingoutputs;
	const InternalKeyComparator internalcomparator;

	// Per level compaction stats.  stats_[level] stores the stats for
	// compactions that produced data for the specified "level".
	struct CompactionStats {
		int64_t micros;
		int64_t bytesread;
		int64_t byteswritten;

		CompactionStats() : micros(0), bytesread(0), byteswritten(0) {}

		void Add(const CompactionStats& c) {
			this->micros += c.micros;
			this->bytesread += c.bytesread;
			this->byteswritten += c.byteswritten;
		}
	};

	CompactionStats stats[kNumLevels];

	// Information for a manual compaction
	struct ManualCompaction {
		int level;
		bool done;
		const InternalKey* begin;   // null means beginning of key range
		const InternalKey* end;     // null means end of key range
		InternalKey tmpStorage;    // Used to keep track of compaction progress
	};

	ManualCompaction* manualcompaction;
	std::mutex mutex;
	std::condition_variable bgfinishedsignal;
	Status bgerror;
};
