/**
 * Design file parser for CNC Chip Carving JSON files
 * Supports design-schema-v2.json format
 */

#pragma once

#include <memory>
#include <string>
#include <vector>
#include <optional>
#include "../geometry/Point2D.h"
#include "../geometry/Shape.h"

namespace ChipCarving {

// Forward declarations
namespace Adapters {
class ILogger;
}

namespace Parsers {

/**
 * Metadata about a design file
 */
struct DesignMetadata {
    std::optional<std::string> name;
    std::optional<std::string> author;
    std::optional<std::string> created;
    std::optional<std::string> modified;
    std::optional<std::string> description;
};

/**
 * Background image data (not used for sketches, but part of schema)
 */
struct BackgroundImage {
    std::string id;
    std::string imageData;  // Base64 encoded
    Geometry::Point2D position;
    double rotation;
    double scale;
    double opacity;
    double naturalWidth;
    double naturalHeight;
};

/**
 * Complete design file contents
 */
struct DesignFile {
    std::string version;
    DesignMetadata metadata;
    std::vector<std::unique_ptr<Geometry::Shape>> shapes;
    std::vector<BackgroundImage> backgroundImages;
};

/**
 * JSON parser for design files
 */
class DesignParser {
   public:
    /**
     * Parse a design file from JSON string
     * @param jsonContent JSON content as string
     * @return Parsed design file
     * @throws std::runtime_error if parsing fails
     */
    static DesignFile parseFromString(const std::string& jsonContent,
                                      const Adapters::ILogger* logger = nullptr);

    /**
     * Parse a design file from file path
     * @param filePath Path to JSON file
     * @return Parsed design file
     * @throws std::runtime_error if file read or parsing fails
     */
    static DesignFile parseFromFile(const std::string& filePath,
                                    const Adapters::ILogger* logger = nullptr);

    /**
     * Validate JSON against schema (basic validation)
     * @param jsonContent JSON content as string
     * @return True if valid, false otherwise
     */
    static bool validateSchema(const std::string& jsonContent);

   private:
    /**
     * Parse metadata section
     */
    static DesignMetadata parseMetadata(const std::string& jsonContent);

    /**
     * Parse shapes array
     */
    static std::vector<std::unique_ptr<Geometry::Shape>> parseShapes(
        const std::string& jsonContent, const Adapters::ILogger* logger = nullptr);

    /**
     * Parse background images array
     */
    static std::vector<BackgroundImage> parseBackgroundImages(const std::string& jsonContent);

    /**
     * Parse a single point from JSON
     */
    static Geometry::Point2D parsePoint(const std::string& pointJson);

    /**
     * Extract string value from JSON
     */
    static std::string extractString(const std::string& json, const std::string& key);

    /**
     * Extract number value from JSON
     */
    static double extractNumber(const std::string& json, const std::string& key);

    /**
     * Extract array from JSON
     */
    static std::string extractArray(const std::string& json, const std::string& key);

    /**
     * Extract object from JSON
     */
    static std::string extractObject(const std::string& json, const std::string& key);
};

}  // namespace Parsers
}  // namespace ChipCarving