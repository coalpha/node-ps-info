{
   "targets": [{
      "target_name": "lib",

      "sources": ["lib.c"],

      # I'm pretty sure this does nothing but whatever
      "conditions":[
         [ "OS=='win'"
         , {"cflags_cc": ["/O2", "/Ot", "/EHc", "/Wall", "/utf-8"]}
         , {"cflags": ["-O2", "-Werror", "-Wextra", "-pedantic"]}
         ]
      ]
   }]
}
