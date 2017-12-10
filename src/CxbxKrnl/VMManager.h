// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com
// ******************************************************************
// *
// *    .,-:::::    .,::      .::::::::.    .,::      .:
// *  ,;;;'````'    `;;;,  .,;;  ;;;'';;'   `;;;,  .,;;
// *  [[[             '[[,,[['   [[[__[[\.    '[[,,[['
// *  $$$              Y$$$P     $$""""Y$$     Y$$$P
// *  `88bo,__,o,    oP"``"Yo,  _88o,,od8P   oP"``"Yo,
// *    "YUMMMMMP",m"       "Mm,""YUMMMP" ,m"       "Mm,
// *
// *   Cxbx->Win32->CxbxKrnl->VMManager.h
// *
// *  This file is part of the Cxbx project.
// *
// *  Cxbx and Cxbe are free software; you can redistribute them
// *  and/or modify them under the terms of the GNU General Public
// *  License as published by the Free Software Foundation; either
// *  version 2 of the license, or (at your option) any later version.
// *
// *  This program is distributed in the hope that it will be useful,
// *  but WITHOUT ANY WARRANTY; without even the implied warranty of
// *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// *  GNU General Public License for more details.
// *
// *  You should have recieved a copy of the GNU General Public License
// *  along with this program; see the file COPYING.
// *  If not, write to the Free Software Foundation, Inc.,
// *  59 Temple Place - Suite 330, Bostom, MA 02111-1307, USA.
// *
// *  (c) 2017      ergo720
// *
// *  All rights reserved
// *
// ******************************************************************

#ifndef VMMANAGER_H
#define VMMANAGER_H


#include "PhysicalMemory.h"


/* VMATypes */

enum class VMAType : u32
{
	// vma represents an unmapped region of the address space
	Free,
	// vma represents allocated memory
	Allocated,
	// vma represents allocated memory mapped outside the second file view (allocated by VirtualAlloc)
	Fragmented,
	// vma represents the xbe sections loaded at 0x10000 (it is counted as allocated memory)
	Xbe,
	// contiguous memory
	MemContiguous,
	// tiled memory
	MemTiled,
	// nv2a
	IO_DeviceNV2A,
	// nv2a pramin memory
	MemNV2A_PRAMIN,
	// apu
	IO_DeviceAPU,
	// ac97
	IO_DeviceAC97,
	// usb0
	IO_DeviceUSB0,
	// usb1
	IO_DeviceUSB1,
	// ethernet controller
	IO_DeviceNVNet,
	// bios
	DeviceBIOS,
	// mcpx rom (retail xbox only)
	DeviceMCPX,
};


/* VirtualMemoryArea struct */

struct VirtualMemoryArea
{
	// vma starting address
	VAddr base = 0;
	// vma size
	size_t size = 0;
	// vma kind of memory
	VMAType type = VMAType::Free;
	// vma permissions
	DWORD permissions = PAGE_NOACCESS;
	// addr of the memory backing this block, if any
	PAddr backing_block = NULL;
	// tests if this area can be merged to the right with 'next'
	bool CanBeMergedWith(const VirtualMemoryArea& next) const;
};


/* VMManager class */

class VMManager : public PhysicalMemory
{
	public:
		// constructor
		VMManager() {};
		// destructor
		~VMManager()
		{
			UnmapViewOfFile((void*)m_Base);
			UnmapViewOfFile((void *)CONTIGUOUS_MEMORY_BASE);
			UnmapViewOfFile((void*)TILED_MEMORY_BASE);
			FlushViewOfFile((void*)m_Base, CHIHIRO_MEMORY_SIZE);
			FlushFileBuffers(m_hAliasedView);
			CloseHandle(m_hAliasedView);
		}
		// initializes the page table to the default configuration
		void Initialize(HANDLE file_view);
		// initialize chihiro - specifc memory ranges
		void InitializeChihiro();
		// print virtual memory statistics
		void VMStatistics() const;
		// creates a vma block to be mapped in memory at the specified VAddr, if requested
		VAddr MapMemoryBlock(size_t size, PAddr low_addr, PAddr high_addr, VAddr addr = NULL);
		// creates a vma representing the memory block to remove
		void UnmapRange(VAddr target);
		// changes access permissions for a range of vma's, splitting them if necessary
		void ReprotectVMARange(VAddr target, size_t size, DWORD new_perms);
		// checks if a VAddr is valid; returns false if not
		bool IsValidVirtualAddress(const VAddr addr);
		// translates a VAddr to its corresponding PAddr; it must be valid
		PAddr TranslateVAddrToPAddr(const VAddr addr);
	
	
	private:
		// m_Vma_map iterator
		typedef std::map<VAddr, VirtualMemoryArea>::iterator VMAIter;
		// map covering the entire 32 bit virtual address space as seen by the guest
		std::map<VAddr, VirtualMemoryArea> m_Vma_map;
		// start address of the memory region to which map new allocations in the virtual space
		VAddr m_Base = 0;
		// handle of the second file view region
		HANDLE m_hAliasedView = NULL;
	
		// maps a new allocation in the virtual address space
		void MapMemoryRegion(VAddr base, size_t size, PAddr target);
		// maps a special allocation outside the virtual address space of the second file view
		void MapSpecialRegion(VAddr base, size_t size, PAddr target);
		// removes an allocation from the virtual address space
		void UnmapRegion(VAddr base, size_t size);
		// removes a vma block from the mapped memory
		VMAIter Unmap(VMAIter vma_handle);
		// updates the page table with a new vma entry
		void MapPages(u32 page_num, u32 page_count, PAddr memory, PTEflags type);
		// carves a vma of a specific size at the specified address by splitting free vma's
		VMAIter CarveVMA(VAddr base, size_t size);
		// splits the edges of the given range of non-free vma's so that there is a vma split at each end of the range
		VMAIter CarveVMARange(VAddr base, size_t size);
		// gets the iterator of a vma in m_Vma_map
		VMAIter GetVMAIterator(VAddr target);
		// splits a parent vma into two children
		VMAIter SplitVMA(VMAIter vma_handle, u32 offset_in_vma);
		// merges the specified vma with adjacent ones if possible
		VMAIter MergeAdjacentVMA(VMAIter iter);
		// changes access permissions for a vma
		VMAIter ReprotectVMA(VMAIter vma_handle, DWORD new_perms);
		// updates the page table
		void UpdatePageTableForVMA(const VirtualMemoryArea& vma);
};


extern VMManager g_VMManager;

#endif