/**
 * DesignParser implementation
 */

#include "../../include/parsers/DesignParser.h"

#include <fstream>
#include <regex>
#include <sstream>
#include <stdexcept>

#include "../../include/geometry/ShapeFactory.h"
#include "../adapters/IFusionInterface.h"

using namespace ChipCarving::Parsers;
using namespace ChipCarving::Geometry;

DesignFile DesignParser::parseFromFile(const std::string& filePath,
                                       const Adapters::ILogger* logger) {
  std::ifstream file(filePath);
  if (!file.is_open()) {
    throw std::runtime_error("Failed to open file: " + filePath);
  }

  std::stringstream buffer;
  buffer << file.rdbuf();
  std::string jsonContent = buffer.str();

  // Debug logging disabled for performance
  // if (logger) {
  //     logger->logDebug("Loaded " + std::to_string(jsonContent.length()) +
  //                      " characters from file");
  // }

  return parseFromString(jsonContent, logger);
}

DesignFile DesignParser::parseFromString(const std::string& jsonContent,
                                         const Adapters::ILogger* logger) {
  DesignFile design;

  // Parse version
  design.version = extractString(jsonContent, "version");
  if (design.version != "2.0") {
    throw std::runtime_error("Unsupported schema version: " + design.version +
                             ". Expected version 2.0");
  }

  // Parse metadata (optional)
  try {
    design.metadata = parseMetadata(jsonContent);
  } catch (const std::exception&) {
    // Metadata is optional, continue if not found
  }

  // Parse shapes (required)
  design.shapes = parseShapes(jsonContent, logger);
  if (design.shapes.empty()) {
    throw std::runtime_error("Design file must contain at least one shape");
  }

  // Parse background images (optional)
  try {
    design.backgroundImages = parseBackgroundImages(jsonContent);
  } catch (const std::exception&) {
    // Background images are optional, continue if not found
  }

  return design;
}

bool DesignParser::validateSchema(const std::string& jsonContent) {
  try {
    // Basic validation - check for required fields
    std::string version = extractString(jsonContent, "version");
    if (version != "2.0") {
      return false;
    }

    // Try to parse shapes array
    std::string shapesArray = extractArray(jsonContent, "shapes");
    if (shapesArray.empty()) {
      return false;
    }

    return true;
  } catch (const std::exception&) {
    return false;
  }
}

DesignMetadata DesignParser::parseMetadata(const std::string& jsonContent) {
  DesignMetadata metadata;

  try {
    std::string metadataJson = extractObject(jsonContent, "metadata");

    try {
      metadata.name = extractString(metadataJson, "name");
    } catch (const std::exception& e) {
      // Optional field - continue without logging
    }
    try {
      metadata.author = extractString(metadataJson, "author");
    } catch (const std::exception& e) {
      // Optional field - continue without logging
    }
    try {
      metadata.created = extractString(metadataJson, "created");
    } catch (const std::exception& e) {
      // Optional field - continue without logging
    }
    try {
      metadata.modified = extractString(metadataJson, "modified");
    } catch (const std::exception& e) {
      // Optional field - continue without logging
    }
    try {
      metadata.description = extractString(metadataJson, "description");
    } catch (const std::exception& e) {
      // Optional field - continue without logging
    }
  } catch (const std::exception&) {
    // Metadata section not found or invalid
  }

  return metadata;
}

std::vector<std::unique_ptr<Shape>> DesignParser::parseShapes(
    const std::string& jsonContent, const Adapters::ILogger* logger) {
  std::vector<std::unique_ptr<Shape>> shapes;

  std::string shapesArray = extractArray(jsonContent, "shapes");

  // Extract individual shape objects from array
  std::regex shapeRegex(R"(\{[^\{\}]*(?:\{[^\{\}]*\}[^\{\}]*)*\})");
  std::sregex_iterator iter(shapesArray.begin(), shapesArray.end(), shapeRegex);
  std::sregex_iterator end;

  for (; iter != end; ++iter) {
    std::string shapeJson = iter->str();
    try {
      auto shape = ShapeFactory::createFromJson(shapeJson, logger);
      shapes.push_back(std::move(shape));
    } catch (const std::exception& e) {
      throw std::runtime_error("Failed to parse shape: " +
                               std::string(e.what()));
    }
  }

  return shapes;
}

std::vector<BackgroundImage> DesignParser::parseBackgroundImages(
    const std::string& jsonContent) {
  std::vector<BackgroundImage> images;

  try {
    std::string imagesArray = extractArray(jsonContent, "backgroundImages");

    // Extract individual image objects from array
    std::regex imageRegex(R"(\{[^\{\}]*(?:\{[^\{\}]*\}[^\{\}]*)*\})");
    std::sregex_iterator iter(imagesArray.begin(), imagesArray.end(),
                              imageRegex);
    std::sregex_iterator end;

    for (; iter != end; ++iter) {
      std::string imageJson = iter->str();
      BackgroundImage image;

      image.id = extractString(imageJson, "id");
      image.imageData = extractString(imageJson, "imageData");
      image.rotation = extractNumber(imageJson, "rotation");
      image.scale = extractNumber(imageJson, "scale");
      image.opacity = extractNumber(imageJson, "opacity");
      image.naturalWidth = extractNumber(imageJson, "naturalWidth");
      image.naturalHeight = extractNumber(imageJson, "naturalHeight");

      // Parse position point
      std::string positionJson = extractObject(imageJson, "position");
      image.position = parsePoint(positionJson);

      images.push_back(image);
    }
  } catch (const std::exception&) {
    // Background images are optional
  }

  return images;
}

Point2D DesignParser::parsePoint(const std::string& pointJson) {
  double x = extractNumber(pointJson, "x");
  double y = extractNumber(pointJson, "y");
  return Point2D(x, y);
}

std::string DesignParser::extractString(const std::string& json,
                                        const std::string& key) {
  std::regex regex("\"" + key + "\"\\s*:\\s*\"([^\"]+)\"");
  std::smatch match;

  if (std::regex_search(json, match, regex)) {
    return match[1].str();
  }

  throw std::runtime_error("String key '" + key + "' not found in JSON");
}

double DesignParser::extractNumber(const std::string& json,
                                   const std::string& key) {
  std::regex regex("\"" + key + "\"\\s*:\\s*([-+]?[0-9]*\\.?[0-9]+)");
  std::smatch match;

  if (std::regex_search(json, match, regex)) {
    return std::stod(match[1].str());
  }

  throw std::runtime_error("Number key '" + key + "' not found in JSON");
}

std::string DesignParser::extractArray(const std::string& json,
                                       const std::string& key) {
  std::regex regex(
      "\"" + key +
      "\"\\s*:\\s*\\[([^\\[\\]]*(?:\\[[^\\[\\]]*\\][^\\[\\]]*)*)\\]");
  std::smatch match;

  if (std::regex_search(json, match, regex)) {
    return match[1].str();
  }

  throw std::runtime_error("Array key '" + key + "' not found in JSON");
}

std::string DesignParser::extractObject(const std::string& json,
                                        const std::string& key) {
  // Find the key and then extract the matching braces
  std::string pattern = "\"" + key + "\"\\s*:\\s*\\{";
  std::regex startRegex(pattern);
  std::smatch match;

  if (!std::regex_search(json, match, startRegex)) {
    throw std::runtime_error("Object key '" + key + "' not found in JSON");
  }

  // Find the start of the object
  size_t start =
      match.position() + match.length() - 1;  // Position of opening brace
  size_t braceCount = 1;
  size_t pos = start + 1;

  // Find matching closing brace
  while (pos < json.length() && braceCount > 0) {
    if (json[pos] == '{') {
      braceCount++;
    } else if (json[pos] == '}') {
      braceCount--;
    }
    pos++;
  }

  if (braceCount != 0) {
    throw std::runtime_error("Malformed JSON object for key '" + key + "'");
  }

  // Extract the object content (without outer braces)
  return json.substr(start + 1, pos - start - 2);
}