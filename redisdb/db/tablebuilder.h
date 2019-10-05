#pragma once

#include <stdint.h>
#include "status.h"
#include "option.h"

class BlockBuilder;

class BlockHandle;

class WritableFile;

class TableBuilder {
public:
	// Create a builder that will store the Contents of the table it is
	// building in *file.  Does not close the file.  It is up to the
	// caller to close the file after calling Finish().
	TableBuilder(const Options& options, const std::shared_ptr<WritableFile>& file);

	TableBuilder(const TableBuilder&) = delete;

	void operator=(const TableBuilder&) = delete;

	~TableBuilder();

	// Add key,value to the table being constructed.
	// REQUIRES: key is after any previously added key according to comparator.
	// REQUIRES: Finish(), Abandon() have not been called
	void Add(const std::string_view& key, const std::string_view& value);

	// Finish building the table.  Stops using the file passed to the
	// constructor after this function returns.
	// REQUIRES: Finish(), Abandon() have not been called
	Status Finish();

	// Advanced operation: flush any buffered key/value pairs to file.
	// Can be used to ensure that two adjacent entries never live in
	// the same data block.  Most clients should not need to use this method.
	// REQUIRES: Finish(), Abandon() have not been called
	void flush();

	// Return non-ok iff some error has been detected.
	Status status() const;

	// Indicate that the Contents of this builder should be abandoned.  Stops
	// using the file passed to the constructor after this function returns.
	// If the caller is not going to call Finish(), it must call Abandon()
	// before destroying this builder.
	// REQUIRES: Finish(), Abandon() have not been called

	void Abandon();

	// Number of calls to Add() so far.
	uint64_t pendinghandle() const;

	// Size of the file generated so far.  If invoked after a successful
	// Finish() call, returns the size of the final generated file.

	uint64_t filesize() const;

private:
	bool ok() const { return status().ok(); }

	void WriteBlock(BlockBuilder* block, BlockHandle* handle);

	void WriteRawBlock(const std::string_view& blockContents,
		CompressionType type, BlockHandle* handle);

	struct Rep;
	std::shared_ptr<Rep> rep;
};
