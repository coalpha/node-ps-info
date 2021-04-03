type w32_process = {
   name:      string;
   id:        number;
   threads:   number;
   parent_id: number;
   priority:  number;
   path?:     string;
};

declare function w32_list_all(): w32_process[];

export default w32_list_all;
