# Threads_scheduling_synchronization
A simple multi-threaded scheduling simulator

To run the program:

schedule <N> <minB> <avgB> <minA> <avgA> <ALG>
  
 <N> is the number of W threads. It can be a value between 1 and 10. 
 A burst time is generated as an exponentially distributed random value, with parameter <avgB>. 
 If the generated random value is less than <minB>, it has to be generated again. 
 Similarly, an interarrival time between two consecutive bursts (i.e., time to sleep between bursts in a W thread) is also to be generated as an exponentially
 distributed random value, with parameter <avgA>. 
 If the generated random value is less than <minA>, it has to be generated again. 
 The <ALG> parameter specifies the scheduling algorithm to use. It can be one of: “FCFS”, “SJF”, “PRIO”, “VRUNTIME”. All simulated scheduling algorithms are non-preemptive.
   
• FCFS: Bursts served in the order they are added to the runqueue.
   
• SJF: When scheduling decision is to be made, the burst that has the smallest burst
  length is selected to run next. But the bursts of a particular thread are
  scheduled in the same order they are generated.
   
• PRIO: A W thread i has priority i. The smaller the number, the higher the priority.
  The burst in the runqueue belonging to the highest priority thread is selected.
  
• VRUNTIME. Each thread is associated with a virtual runtime (vruntime). When a
  burst of a thread is executed, the vruntime of the thread is advanced by some
  amount. The amount depends on the priority of the thread. When thread i runs t
  ms, its virtual runtime is advanced by t(0.7 + 0.3i). For example, virtual runtime
  of thread 1 is advanced by t ms when it runs t ms in cpu; the virtual runtime of
  thread 11 is advanced by 4t when it runs t ms in cpu. When scheduling decision is
  to be made, the burst of the thread that has the smallest virtual runtime is selected
  for execution. Again we can not reorder the bursts of a particular thread. They
  need to be served in the order they are added to the runqueue. 
   
# Example invocations of the program
   
   schedule 3 100 200 1000 1500 FCFS
   OR
   schedule 5 FCFS -f infile
   
   
