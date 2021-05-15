{
   "targets": [{
      "target_name": "ps-list",
      "sources": ["lib.c"],

      # I'm pretty sure this does nothing but whatever
      "conditions":[
         [ "OS=='win'"
         , {"cflags_cc": ["/O2", "/Ot", "/EHc", "/Wall", "/utf-8"]}
         , {"cflags": ["-O2", "-Wall", "-Wextra"]}
         ]
      ]
   }]
}
