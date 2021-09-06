#include <algorithm>
#include <iomanip>
#include <iostream>
#include <numeric>
#include <queue>
#include <stack>
#include <string>

class graph {
public:
  struct edge {
    std::size_t src;
    std::size_t dest;
    int         weight;

    [[nodiscard]] edge reverse() const {
      edge rev = *this;
      std::swap(rev.src, rev.dest);
      return rev;
    }
  };

  graph() = default;

  explicit graph(std::vector<edge> edges, bool directed = true)
      : edges_(std::move(edges)), node_count_(node_count(edges_)) {

    adj_list_.resize(node_count_);
    for (auto& e: edges_) {
      adj_list_[e.src].push_back(e);
      if (!directed) {
        edge r = e.reverse();
        adj_list_[r.src].push_back(r);
      }
    }
  }

  struct node {
    std::size_t node;
    int         weight;
    bool        operator<(const struct node& rhs) const { return weight < rhs.weight; }
  };

  struct node_info {
    std::size_t prev = std::numeric_limits<std::size_t>::max();
    int         dist = std::numeric_limits<int>::max();
    bool        done = false;
  };

  [[nodiscard]] std::vector<node_info> shortest_path(std::size_t source) const {
    std::vector<node_info> ni(node_count_);
    ni[source].dist = 0;

    std::priority_queue<node> q;
    q.push({source, 0});

    while (!q.empty()) {
      auto n = q.top();
      q.pop();

      std::size_t u = n.node;
      if (ni[u].done) continue; // skip stale repeats
      ni[u].done = true;

      for (const auto& edge: adj_list_[u]) {
        std::size_t v      = edge.dest;
        int         weight = edge.weight;

        if (!ni[v].done && (ni[u].dist + weight) < ni[v].dist) {
          ni[v].dist = ni[u].dist + weight; // update cost
          ni[v].prev = u;                   // record path back
          q.push({v, ni[v].dist});          // enqueue
        }
      }
    }
    return ni;
  }

  void depth_first_traversal(std::size_t src) {
    std::stack<std::size_t> s;

    std::vector<bool> visted(node_count_);

    s.push(src);
    while (!s.empty()) {
      auto cur = s.top();
      s.pop();
      if (visted[cur]) continue;
      visted[cur] = true;
      std::cout << cur << " ";
      for (auto it = rbegin(adj_list_[cur]); it != rend(adj_list_[cur]); ++it)
        if (!visted[it->dest]) s.push(it->dest);
    }
    std::cout << "\n";
  }

  void breadth_first_traversal(std::size_t src) {
    std::queue<std::size_t> q;
    std::vector<bool>       visted(node_count_);

    q.push(src);
    visted[src] = true;
    while (!q.empty()) {
      auto cur = q.front();
      q.pop();
      std::cout << cur << " ";

      for (auto&& e: adj_list_[cur]) {
        if (!visted[e.dest]) {
          visted[e.dest] = true;
          q.push(e.dest);
        }
      }
    }
    std::cout << "\n";
  }

  void breadth_first_traversal_with_levels(std::size_t src) {
    std::queue<std::size_t> q;
    std::vector<bool>       visted(node_count_);
    std::vector<int>        level(node_count_, -1);

    q.push(src);
    visted[src]  = true;
    int curlevel = 0;
    level[src]   = 0;
    while (!q.empty()) {
      auto cur = q.front();
      q.pop();
      if (level[cur] > curlevel) {
        std::cout << "\n";
        curlevel = level[cur];
      }
      std::cout << cur << " ";

      for (auto&& e: adj_list_[cur]) {
        if (!visted[e.dest]) {
          visted[e.dest] = true;
          q.push(e.dest);
          level[e.dest] = level[cur] + 1;
        }
      }
    }
    std::cout << "\n";
  }

  static void print_result(const std::vector<node_info>& ni, std::size_t source) {
    // print result
    std::size_t N = ni.size();
    for (std::size_t n = 0; n < N; ++n) {
      if (ni[n].done && n != source) {
        std::cout << source << " -> " << n << ": cost=" << ni[n].dist << ", path: ";
        print_path(ni, n);
        std::cout << "\n";
      }
    }
  }

  [[nodiscard]] std::size_t size() const { return node_count_; }

  friend std::ostream& operator<<(std::ostream& os, const graph& g) {
    os << g.size() << " vertices, " << g.edges_.size() << " edges\n";
    for (std::size_t n = 0; n < g.node_count_; ++n) {
      if (g.adj_list_[n].empty())
        os << "(" << n << ")";
      else
        for (const auto& p: g.adj_list_[n])
          os << "(" << p.src << ", " << p.dest << ", " << p.weight << ") ";
      os << "\n";
    }
    return os;
  }

private:
  std::vector<edge>              edges_;
  std::vector<std::vector<edge>> adj_list_;
  std::size_t                    node_count_ = 0;

  static std::size_t node_count(const std::vector<edge>& edges) {
    std::vector<std::size_t> nodes;
    std::for_each(edges.begin(), edges.end(), [&nodes](const edge& e) {
      nodes.push_back(e.src);
      nodes.push_back(e.dest);
    });
    std::sort(nodes.begin(), nodes.end());
    nodes.erase(std::unique(nodes.begin(), nodes.end()), nodes.end());
    if (!nodes.empty() && (nodes.front() != 0 || nodes.back() != nodes.size() - 1))
      throw std::logic_error("Nodes must be numbered 0 -> N-1");
    return nodes.size();
  }

  static void print_path(const std::vector<node_info>& ni, std::size_t i) {
    if (ni[i].prev == std::numeric_limits<std::size_t>::max()) {
      std::cout << i << " ";
      return;
    }

    print_path(ni, ni[i].prev);
    std::cout << i << " ";
  }
};

std::vector<int> get_vector() {
  std::vector<int> vec(5);
  std::cout << &vec << "\n";
  std::iota(begin(vec), end(vec), 0);
  return vec;
}

int main() {
  graph g(
      {
          {0, 1, 1},
          {0, 2, 1},
          {0, 3, 1},
          {1, 4, 1},
          {1, 5, 1},
          {2, 6, 1},
          {2, 7, 1},
          {2, 8, 1},
          {3, 9, 1},

          // {0, 1, 4},
          //   {0, 7, 4},
          //   {1, 2, 8},
          //   {1, 7, 11},
          //   {2, 8, 2},
          //   {2, 5, 4},
          //   {2, 3, 7},
          //   {3, 5, 14},
          //   {3, 4, 9},
          //   {4, 5, 10},
          //   {5, 6, 2},
          //   {6, 8, 6},
          //   {6, 7, 1},
          //   {7, 8, 7},
      },
      false);
  std::cout << g;
  // std::size_t source = 0;
  // auto        result = g.shortest_path(source);
  // g.print_result(result, source);
  g.depth_first_traversal(0);
  g.breadth_first_traversal(0);
}
