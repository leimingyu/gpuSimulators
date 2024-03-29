/*
 *  Multi2Sim
 *  Copyright (C) 2012  Rafael Ubal (ubal@ece.neu.edu)
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef ARCH_SOUTHERN_ISLANDS_EMU_WORK_GROUP_H
#define ARCH_SOUTHERN_ISLANDS_EMU_WORK_GROUP_H

#include <cassert>
#include <memory>
#include <vector>

#include <mem-system/Memory.h>


namespace SI
{

class Wavefront;
class WorkItem;
class NDRange;

const unsigned wavefront_size = 64;

/// This is a polymorphic class used to attach additional information
/// to a work-group. It is used by the timing simulator to associate timing
/// simulation information per work-group.
class WorkGroupData
{
public:
	/// Virtual destructor to guarantee polymorphism
	virtual ~WorkGroupData() { };
};


/// This class represents an OpenCL work-group, as set up by the host program
/// running on Multi2Sim with a call to \a clEnqueueNDRangeKernel(). The
/// work-group is composed of a set of work-items (grouped in chunks of 64,
/// forming wavefronts). Work-groups can run in any order.
class WorkGroup
{
	// Identifiers
	int id;
	int id_3d[3];

	// Status
	unsigned wavefronts_at_barrier;
	unsigned wavefronts_completed_emu;
	unsigned wavefronts_completed_timing;
	bool finished_emu;
	bool finished_timing;

	// ND-Range that it belongs to
	NDRange *ndrange;

	// Counter of workitems in the work-group
	unsigned work_item_count;

	// List of work-items in the work-group
	std::vector<std::unique_ptr<WorkItem>> work_items;

	// List of wavefronts in the work-group
	std::vector<std::unique_ptr<Wavefront>> wavefronts;

	// Local memory
	std::unique_ptr<mem::Memory> lds;

	// Additional work-group data
	std::unique_ptr<WorkGroupData> data;

	// Statistics
	long long sreg_read_count;
	long long sreg_write_count;
	long long vreg_read_count;
	long long vreg_write_count;

public:

	/// Constructor
	///
	/// \param ndrange Instance of class NDRange that it belongs to.
	/// \param id Work-group global 1D identifier
	WorkGroup(NDRange *ndrange, unsigned id);

	/// Dump work-group in human readable format into output stream
	void Dump(std::ostream &os) const;

	/// Equivalent to WorkGroup::Dump()
	friend std::ostream &operator<<(std::ostream &os,
			const WorkGroup &work_group) {
		work_group.Dump(os);
		return os;
	}


	/// Getters
	///
	/// Get workgroup ID
	int getId() const { return id; }

	/// Get workgroup ID in 3D
	int getId3D(unsigned dim) {
		assert(dim >= 0 && dim <= 2);
		return id_3d[dim];		
	}

	/// Get counter of wavefronts in workgroup
	unsigned getWavefrontsInWorkgroup() const { return wavefronts.size(); }

	/// Get counter of wavefronts at barrier
	unsigned getWavefrontsAtBarrier() const { return wavefronts_at_barrier; }

	/// Get counter of finished wavefront, emulation mode
	unsigned getWavefrontsCompletedEmu() const { return wavefronts_completed_emu; }

	/// Get counter of finished wavefront, timing mode
	unsigned getWavefrontsCompletedTiming() const { return wavefronts_completed_timing; }	

	/// Return value of sreg_read_count
	long long getSregReadCount() const{ return sreg_read_count; }

	/// Return value of sreg_write_count
	long long getSregWriteCount() const { return sreg_write_count; }

	/// Return value of vreg_read_count
	long long getVregReadCount() const { return vreg_read_count; }

	/// Return value of vreg_read_count
	long long getVregWriteCount() const { return vreg_write_count; }

	/// Get NDRange that it belongs to
	NDRange *getNDRange() const { return ndrange; }

	/// Get pointer of a workitem in this workgroup
	WorkItem *getWorkItem(int id_in_work_group) {
		assert(id_in_work_group >= 0 && id_in_work_group < (int)work_items.size());
		return work_items[id_in_work_group].get();
	}

	/// Setters
	///
	/// Increase wavefronts_at_barrier counter
	void incWavefrontsAtBarrier() { wavefronts_at_barrier++; }

	/// Increase wavefronts_completed_emu counter
	void incWavefrontsCompletedEmu() { wavefronts_completed_emu++; }

	/// Increase scalar register read counter
	void incSregReadCount() { sreg_read_count++; }

	/// Increase scalar register write counter
	void incSregWriteCount() { sreg_write_count++; }

	/// Increase vector register read counter
	void incVregReadCount() { vreg_read_count++; }

	/// Increase vector register write counter
	void incVregWriteCount() { vreg_write_count++; }

	/// Set wavefront_at_barrier counter
	void setWavefrontsAtBarrier(unsigned counter) { wavefronts_at_barrier = counter; }

	/// Attach additional data to the work-group, passing an object derived
	/// from class WorkGroupData. The object passed to it must be
	/// dynamically allocated with \a new. The WorkGroup object will take
	/// ownership of that pointer and free it automatically when the
	/// work-group is destructed.
	void setData(WorkGroupData *data);

	/// Set finished_emu flag
	void setFinishedEmu(bool flag) { finished_emu = flag; }

	/// Return an iterator to the first work-item in the work-group. The
	/// following code can then be used to iterate over all work-items (and
	/// print them)
	///
	/// \code
	/// for (auto i = work_group->WorkItemsBegin(),
	///		e = work_group->WorkItemsEnd(); i != e; ++i)
	///	i->Dump(std::cout);
	/// \endcode
	std::vector<std::unique_ptr<WorkItem>>::iterator WorkItemsBegin() {
		return work_items.begin();
	}
	
	/// Return a past-the-end iterator to the list of work-items
	std::vector<std::unique_ptr<WorkItem>>::iterator WorkItemsEnd() {
		return work_items.end();
	}

	/// Return an iterator to the first wavefront in the work-group. The
	/// following code can then be used to iterate over all wavefronts (and
	/// print them)
	///
	/// \code
	/// for (auto i = work_group->WavefrontsBegin(),
	///		e = work_group->WavefrontsEnd(); i != e; ++i)
	///	i->Dump(std::cout);
	/// \endcode
	std::vector<std::unique_ptr<Wavefront>>::iterator WavefrontsBegin() {
		return wavefronts.begin();
	}
	
	/// Return a past-the-end iterator to the list of wavefronts
	std::vector<std::unique_ptr<Wavefront>>::iterator WavefrontsEnd() {
		return wavefronts.end();
	}
};

}  // namespace

#endif
