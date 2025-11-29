/**
 * SVGGeneratorComparator.cpp
 *
 * SVG comparison functionality for testing and validation.
 * Split from SVGGenerator.cpp for maintainability
 */

#include <algorithm>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <regex>

#include "geometry/SVGGenerator.h"

using ChipCarving::Geometry::SVGComparator;

// SVGComparator implementation
bool SVGComparator::compare(const std::string& file1, const std::string& file2, double tolerance) {
  std::ifstream f1(file1), f2(file2);
  if (!f1.is_open() || !f2.is_open()) {
    return false;
  }

  std::string content1((std::istreambuf_iterator<char>(f1)), std::istreambuf_iterator<char>());
  std::string content2((std::istreambuf_iterator<char>(f2)), std::istreambuf_iterator<char>());

  auto numbers1 = extractNumbers(content1);
  auto numbers2 = extractNumbers(content2);

  return compareNumbers(numbers1, numbers2, tolerance);
}

std::vector<double> SVGComparator::extractNumbers(const std::string& svgContent) {
  std::vector<double> numbers;

  // Regex to find floating point numbers in the SVG
  std::regex numberRegex(R"([-+]?[0-9]*\.?[0-9]+([eE][-+]?[0-9]+)?)");
  std::sregex_iterator iter(svgContent.begin(), svgContent.end(), numberRegex);
  std::sregex_iterator end;

  for (; iter != end; ++iter) {
    try {
      double value = std::stod(iter->str());
      numbers.push_back(value);
    } catch (const std::exception& e) {
      (void)e;  // Skip invalid numbers
    }
  }

  return numbers;
}

bool SVGComparator::compareNumbers(const std::vector<double>& numbers1, const std::vector<double>& numbers2,
                                   double tolerance) {
  if (numbers1.size() != numbers2.size()) {
    return false;
  }

  for (size_t i = 0; i < numbers1.size(); ++i) {
    if (std::abs(numbers1[i] - numbers2[i]) > tolerance) {
      return false;
    }
  }

  return true;
}
