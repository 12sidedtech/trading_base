The Megaqueue
======

A Megaqueue is an extremely large (> 2GB) virtual memory allocation that has
only been backed by physical pages for an extremely limited active window of
the region. The Megaqueue is typically used when you will not be flushing
chunks of the queue to disk, but there is a convenient usage mode where the
megaqueue monitor will force pdflush to dump the pages to disk.

What is `mqmon` ?
-----

A Megaqueue isn't standalone. Typically, Megaqueues are used by one or more
consumers to access some shared state that is provided by a component of a
system built on the Trading Standard library. As an example, a Megaqueue can
be used to decouple reads and writes to the network, across separate threads
or even processes.

A separate, standalone process, called `mqmon`, will subscribe to all megaqueues
on a given system. The `mqmon` process will periodically wake up and check that
each megaqueue has a particular amount of free physical space, and will add pages
to any megaqueue that is hitting the upper bound of its physical allocation.
Note that a megaqueue producer process will never fail because of a `mqmon`
failure, but rather will slow down a bit due to the process itself having to take
a page fault to populate the virtual mappings with physical pages.

In addition, `mqmon` will use `madvise(2)` to periodically force the system to
drop the physical pages that back inactive regions of the megaqueue, in an
effort to conserve system resources. Megaqueue-consumer applications are expected
to avoid any sort of backtracking in the queue (in fact, this is unsupported and
may result in garbage data).

Why not do this in the kernel?
-----

Good question. Why not indeed? Mostly reliability reasons, but also time to
market. Eventually this should run in a similar fashion to what pdflush does,
as a periodic thread in the kernel's context.

