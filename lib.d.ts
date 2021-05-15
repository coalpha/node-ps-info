interface process {
   /** process id */
   pid: number;
   /** executable or process name */
   name: string;
   /** parent process id */
   ppid: number;

   /** state of process */
   state: string
   /** number of threads */
   threads: number;
   /** priority */
   priority: number;
}

interface process_with_path extends process {
   path?: string;
}

declare function process_list(with_paths: false): process[];
declare function process_list(with_paths: true ): process_with_path[];

export = process_list;
