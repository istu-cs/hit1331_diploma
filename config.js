{
    "service" : {
        "api" : "http",
        "port" : 8080
    },
    "logging" : {
        "level": "debug"
    },
    "http" : {
        "script_names" : [ "/monitor" ]
    },
    "file_server" : {
        "enable": true,
        "document_root_" : "/home/lex/dev/hit1331/diploma"
    },
    "views" : {
        "paths": [ "build" ],
        "skins": [ "plain" ]
    },
    "mon" : {
        "db": "postgresql:host=localhost;user=hit1331;password=hit1331_pwd;dbname=hit1331_diploma"
    }
}
