#pragma once

#include <algorithm>
#include <string>
#include <vector>

inline std::string toupper(std::string s) {
  std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return std::toupper(c); });
  return s;
}

inline void ltrim(std::string& s) { s.erase(0, s.find_first_not_of(" \t\n\v\f\r")); }
inline void rtrim(std::string& s) { s.erase(s.find_last_not_of(" \t\n\v\f\r") + 1); }
inline void trim(std::string& s) {
  ltrim(s);
  rtrim(s);
}

inline unsigned edit_distance(const std::string& s1, const std::string& s2) {
  const std::size_t                      len1 = s1.size();
  const std::size_t                      len2 = s2.size();
  std::vector<std::vector<unsigned>> d(len1 + 1, std::vector<unsigned>(len2 + 1));

  d[0][0] = 0;
  for (unsigned i = 1; i <= len1; ++i) d[i][0] = i;
  for (unsigned i = 1; i <= len2; ++i) d[0][i] = i;

  for (unsigned i = 1; i <= len1; ++i)
    for (unsigned j = 1; j <= len2; ++j)
      d[i][j] = std::min({d[i - 1][j] + 1, d[i][j - 1] + 1, d[i - 1][j - 1] + (s1[i - 1] == s2[j - 1] ? 0 : 1)});
  return d[len1][len2];
}
