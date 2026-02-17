#pragma once
#include "audio_interface.h"
#include <vector>
#include <memory>
#include <map>
#include <algorithm>
#include <cctype>

class AudioFormatFactory {
private:
    std::vector<std::unique_ptr<IAudioFormat>> formats;
    std::map<std::string, IAudioFormat*> extensionMap;
    
    static AudioFormatFactory* instance;
    AudioFormatFactory() = default;
    
    std::string toLower(const std::string& str) const {
        std::string result = str;
        std::transform(result.begin(), result.end(), result.begin(), ::tolower);
        return result;
    }
    
    std::string getFileExtension(const std::string& filename) const {
        size_t dotPos = filename.find_last_of('.');
        if (dotPos == std::string::npos) return "";
        return toLower(filename.substr(dotPos + 1));
    }
    
public:
    static AudioFormatFactory& getInstance() {
        static AudioFormatFactory instance;
        return instance;
    }
    
    void registerFormat(std::unique_ptr<IAudioFormat> format) {
        if (!format) return;
        
        IAudioFormat* formatPtr = format.get();
        formats.push_back(std::move(format));
        
        for (const std::string& ext : formatPtr->getSupportedExtensions()) {
            std::string lowerExt = toLower(ext);
            if (lowerExt.empty()) continue;
            
            if (extensionMap.find(lowerExt) == extensionMap.end() || 
                formatPtr->getPriority() > extensionMap[lowerExt]->getPriority()) {
                extensionMap[lowerExt] = formatPtr;
            }
        }
    }
    
    std::unique_ptr<IAudioReader> createReader(const std::string& filename) {
        std::string extension = getFileExtension(filename);
        
        // Try extension-based lookup first
        auto it = extensionMap.find(extension);
        if (it != extensionMap.end()) {
            auto reader = it->second->createReader();
            if (reader && reader->canHandle(filename)) {
                return reader;
            }
        }
        
        // Fall back to checking all formats
        for (const auto& format : formats) {
            if (format->canHandle(filename)) {
                auto reader = format->createReader();
                if (reader) {
                    return reader;
                }
            }
        }
        
        return nullptr;
    }
    
    std::unique_ptr<IAudioWriter> createWriter(const std::string& filename) {
        std::string extension = getFileExtension(filename);
        
        auto it = extensionMap.find(extension);
        if (it != extensionMap.end()) {
            return it->second->createWriter();
        }
        
        return nullptr;
    }
    
    std::vector<std::string> getSupportedFormats() const {
        std::vector<std::string> result;
        for (const auto& format : formats) {
            result.push_back(format->getFormatName());
        }
        return result;
    }
    
    std::vector<std::string> getSupportedExtensions() const {
        std::vector<std::string> result;
        for (const auto& pair : extensionMap) {
            result.push_back(pair.first);
        }
        return result;
    }
    
    bool isFormatSupported(const std::string& filename) const {
        std::string extension = getFileExtension(filename);
        return extensionMap.find(extension) != extensionMap.end();
    }
};
