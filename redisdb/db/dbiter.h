#pragma once

#include <stdint.h>
#include <random>
#include <vector>
#include <memory>
#include <list>
#include <any>
#include <assert.h>

#include "dbformat.h"
#include "iterator.h"

class DB;

// Return a new iterator that converts internal keys (yielded by
// "*internal_iter") that were live at the specified "sequence" number
// into appropriate user keys.
std::shared_ptr<Iterator> NewDBIterator(DB* db,
	const Comparator* usercmp,
	std::shared_ptr<Iterator> internaliter,
	uint64_t sequence,
	uint32_t seed);



