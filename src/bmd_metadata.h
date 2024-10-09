// red.h
#include <map>
#include <string>

#include "BlackmagicRawAPI.h"

std::map<std::string, std::string> read_metadata(const std::string filename);

std::map<std::string, Variant> ProcessClip(CFStringRef clipName, CFStringRef binaryDirectoryCFStr);

std::map<std::string, Variant> GetMetadataMap(IBlackmagicRawMetadataIterator* metadataIterator);

void PrintVariant(const Variant& value, std::ostringstream& oss);