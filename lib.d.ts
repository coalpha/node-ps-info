interface process {
   name:      string;
   id:        number;
   threads:   number;
   parent_id: number;
   priority:  number;
}

interface process_with_path extends process {
   path?: string;
}

declare function process_list(with_paths: false): process[];
declare function process_list(with_paths: true ): process_with_path[];

export = process_list;
