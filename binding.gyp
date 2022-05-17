{
  "targets": [
    {
      "target_name": "main",
      "sources": [ "src/main.cpp", "src/LiveSplitReader.cpp", "src/include/pugixml.cpp" ],
      'defines': [ 'NAPI_DISABLE_CPP_EXCEPTIONS' ],
      "libraries": ["../src/libs/steam_api64.lib", "../src/libs/steam_api.lib"],
      'include_dirs': ["<!(node -p \"require('node-addon-api').include_dir\")", "../src/include"],
      "conditions": [
        ["OS==\"win\"", {
            "copies": [
                {
                    "destination": "<(module_root_dir)/build/Release/",
                    "files": ["<(module_root_dir)/src/libs/steam_api64.dll", "<(module_root_dir)/src/libs/steam_api.dll"]
                }
            ]
        }]
      ]
    }
  ],
}