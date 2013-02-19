contain
=======

A wrapper that lets you tweak Linux process options before you start a process.
Kinda like a super version of nice(1).  In particular this lets you create
process namespaces and cgroups which lets you form up fairly usable containers.

Examples
-------

* contain --ioidle=0 -- ls -R /  
Run ls in the background with the io priority set to idle.  Using ioidle in this
way is great for programs that read a lot of data (but not necessarily write a
lot of data, see set\_ioprio(2) for limitations)
 
* sudo contain --newnet -- bash  
Create a new networking namespace and run bash inside of it.

* Creating a new pid namespace:
 * sudo contain --newpid --newmount -- sh -c 'mount -t proc none /proc ; ps aux'

To demonstrate this, we need to create a new mount namespace too, otherwise you
still see the proc filesystem from outside the namespace and thus ps will give
you weird results.

* Using containers:
 * # First setup cgroups on this machine
 * sudo mount -t tmpfs none /sys/fs/cgroup/
 * sudo mkdir /sys/fs/cgroup/devices
 * sudo mount -t cgroup -o devices devices /sys/fs/cgroup/devices
 * # Now demonstrate their use
 * sudo contain --name "example-cgroup" \\   
  --limit devices.deny=a \\   
  --limit devices.allow="c 1:3 mr" \\  
  -- cat /dev/null
 * sudo contain --name "example-cgroup" \\  
  --limit devices.deny=a \\  
  --limit devices.allow="c 1:3 mr" \\  
  -- cat /dev/zero  
This demonstrates a simple container that lets you specify which device nodes
processes have access to.  This denies access to all devices, but allows access
to /dev/null.  The two commands above demonstrate /dev/null working, but
/dev/zero failing with operation not permitted.

* Using capabilities
  * This doesn't seem to work yet for some reason.

* Making root a non priviledged user
   sudo contain --noroot --uid=nobody -- su -c id  
This demonstrates that nobody can execute su, but the setuid bit doesn't grant
it any extra privileges




