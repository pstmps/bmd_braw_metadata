%module bmd_metadata

%{
#include "bmd_metadata.cpp"
%}

%include "std_string.i"
%include "std_map.i"

%template(StringMap) std::map<std::string, std::string>;

extern std::map<std::string, std::string> read_metadata(const std::string filename, const std::string binaryDirectory);

// 0.2.0 removed boost dependency

// build with:
// clang++ -Wno-everything -Wc++11-extensions -shared -fPIC bmd_metadata_wrap.cxx ./Include/BlackmagicRawAPIDispatch.cpp -I./Include -I./Libraries -I/Library/Frameworks/Python.framework/Versions/3.9/include/python3.9 -F./Libraries -framework BlackmagicRawAPI -framework CoreFoundation -L/Library/Frameworks/Python.framework/Versions/3.9/lib -lpython3.9 -o _bmd_metadata.so -mmacosx-version-min=10.13


// 0.1.0
// build with: 
// clang++ -Wno-everything -Wc++11-extensions -shared -fPIC bmd_metadata_wrap.cxx ./Include/BlackmagicRawAPIDispatch.cpp -I./Include -I./Libraries -I/Library/Frameworks/Python.framework/Versions/3.9/include/python3.9 -F./Libraries -framework BlackmagicRawAPI -framework CoreFoundation -L/Library/Frameworks/Python.framework/Versions/3.9/lib -lpython3.9 -lboost_filesystem -o _bmd_metadata.so

// or with macos version limit:
// clang++ -Wno-everything -Wc++11-extensions -shared -fPIC bmd_metadata_wrap.cxx ./Include/BlackmagicRawAPIDispatch.cpp -I./Include -I./Libraries -I/Library/Frameworks/Python.framework/Versions/3.9/include/python3.9 -F./Libraries -framework BlackmagicRawAPI -framework CoreFoundation -L/Library/Frameworks/Python.framework/Versions/3.9/lib -lpython3.9 -lboost_filesystem -o _bmd_metadata.so -mmacosx-version-min=10.13

// def read_metadata(filename: str) -> dict:
    // """
    // Read metadata from a Blackmagic RAW (.braw) file.
    // Args:
    //     filename: The path to the file to read.
    // Returns:
    //     A dictionary of metadata.
    // """
    // metadata_raw = _bmd_metadata.read_metadata(filename)
    // metadata = {}
    // for key, value in metadata_raw.items():
    //     metadata[key] = (value.encode('utf-8', 'ignore').decode('utf-8'))
    // return metadata