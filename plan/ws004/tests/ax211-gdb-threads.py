# gdb Python helper (ws004-p051): lists every process/thread of a stopped
# zedBSD amd64 guest and backtraces each thread that is not on a CPU from the
# context asm_task_dispatch saved.  Needs a vmunix built with DWARF.
#   gdb vmunix -ex 'target remote :PORT' -x ax211-gdb-threads.py -ex 'zthreads'
import gdb

SAVED = ['r15', 'r14', 'r13', 'r12', 'rbp', 'rbx', 'eflags']


class ZThreads(gdb.Command):
    def __init__(self):
        super().__init__('zthreads', gdb.COMMAND_USER)

    def invoke(self, arg, from_tty):
        depth = int(arg) if arg else 12
        regs = {r: int(gdb.parse_and_eval('$' + r)) for r in
                ['rsp', 'rip', 'rbp', 'rbx', 'r12', 'r13', 'r14', 'r15']}
        try:
            proc = gdb.parse_and_eval('all_processes')
            while int(proc) != 0:
                pid = int(proc['pid'])
                thr = proc['threads']
                while int(thr) != 0:
                    self.show(pid, thr, depth)
                    thr = thr['proc_next']
                proc = proc['all_next']
        finally:
            for r, v in regs.items():
                gdb.execute('set $%s = %d' % (r, v), to_string=True)

    def show(self, pid, thr, depth):
        state = str(thr['state'])
        entry = thr['kernel_entry']
        task = thr['task'].cast(gdb.lookup_type('struct amd64_task').pointer())
        print('pid %d tid %d %s entry=%s task=%s run_cpu=%d' % (
            pid, int(thr['tid']), state, entry, task, int(task['run_cpu'])))
        if state == 'THREAD_RUNNING':
            return
        rsp = int(task['resume_rsp'])
        if rsp == 0:
            return
        inferior = gdb.selected_inferior()
        words = [int.from_bytes(inferior.read_memory(rsp + 8 * i, 8).tobytes(),
                                'little') for i in range(8)]
        for i, r in enumerate(['r15', 'r14', 'r13', 'r12', 'rbp', 'rbx']):
            gdb.execute('set $%s = %d' % (r, words[i]), to_string=True)
        gdb.execute('set $rip = %d' % words[7], to_string=True)
        gdb.execute('set $rsp = %d' % (rsp + 64), to_string=True)
        try:
            print(gdb.execute('bt %d' % depth, to_string=True))
        except gdb.error as error:
            print('  bt failed: %s' % error)


ZThreads()
