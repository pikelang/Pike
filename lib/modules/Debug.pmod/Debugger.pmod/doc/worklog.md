Microsoft's Debug Adapter Procotol specification downloaded on January 16th, 2019 from:
https://raw.githubusercontent.com/Microsoft/debug-adapter-protocol/gh-pages/debugAdapterProtocol.json

Project's home page:
https://microsoft.github.io/debug-adapter-protocol/


DebugAdapterProtocol.pmod generateby by quicktype with a little hack regarding enum generation.

Branch with hacked Pike Renderer:
https://github.com/mkrawczuk/quicktype/tree/protocol_schema

After installation, run the following command from the project's top catalog:
$ ./script/quicktype -s schema "https://raw.githubusercontent.com/Microsoft/vscode-debugadapter-node/master/debugProtocol.json#/definitions/" -o DebugAdapterProtocol.pmod

