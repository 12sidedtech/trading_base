The TSL Allocator
=====

The TSL allocators are 100% constant time for all operations involved in allocating/deallocating objects. A single
allocator is only able to contain references to objects all of the same time. An allocator, essentially, behaves
as a growable/shrinkable arena of pages that can be used to simplify the lifecycle of memory management.

Tunable Parameters
---

The allocator provides a few tunable parameters, passed to the application as environment variables:

###`TSL_NR_SLABS`
Tunable that allows specifying the number of slabs to be pre-allocated by the allocator subsystem. This number cannot
grow once the application has been started, thus should be set conservatively high.

These slabs are 4kB in size, typical system page size.

###`TSL_NR_HUGE_SLABS`
Tunable that allows specifying the number of huge page slabs to be pre-allocated by the allocator subsystem. This number
cannot grow once the application has started, thus should be set conservatively high. Note that huge pages are typically
a scarce resource on a system, but availability can be configured by setting `/proc/mem/nr_hugepages` appropriately:

`echo -n 32 > /proc/sys/vm/nr_hugepages`

This would make 32 huge pages available to the system. Typical production systems will require a larger number of
huge page resources, however.

These slabs are usually 2MB in size, the x86_64 system huge page size.
