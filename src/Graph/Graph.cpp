#include "gross/Graph/Graph.h"
#include "gross/Graph/BGL.h"
#include "boost/graph/graphviz.hpp"
#include <algorithm>
#include <iterator>

using namespace gross;

// put here since lazy_edge_iterator is declared
// out-of-line
Graph::edge_iterator Graph::edge_begin() { return edge_iterator(this); }
Graph::edge_iterator Graph::edge_end() { return edge_iterator(this, true); }
size_t Graph::edge_size() { return std::distance(edge_begin(),
                                                 edge_end()); }

SubGraph::edge_iterator SubGraph::edge_begin() { return edge_iterator(this); }
SubGraph::edge_iterator SubGraph::edge_end() {
  return edge_iterator(this, true);
}
size_t SubGraph::edge_size() { return std::distance(edge_begin(),
                                                    edge_end()); }

void Graph::InsertNode(Node* N) {
  Nodes.emplace_back(N);
}

void Graph::AddSubRegion(const SubGraph& SG) {
  SubRegions.push_back(SG);
}
void Graph::AddSubRegion(SubGraph&& SG) {
  SubRegions.push_back(SG);
}

void Graph::dumpGraphviz(std::ostream& OS) {
  boost::write_graphviz(OS, *this,
                        graph_vertex_prop_writer(*this),
                        graph_edge_prop_writer{},
                        graph_prop_writer{});
}

// Add nodes recursively
SubGraph::SubGraph(Node* EndNode) {
  assert(EndNode);
  std::vector<Node*> BFSQueue;
  BFSQueue.push_back(EndNode);
  while(!BFSQueue.empty()) {
    Node* N = BFSQueue.front();
    assert(N);
    BFSQueue.erase(BFSQueue.cbegin());

    // TODO: here we don't consider call, which
    // will have a value dep to the callee func
    Nodes.push_back(N);
    for(auto* IN : N->inputs())
      BFSQueue.push_back(IN);
  }
}

bool SubGraph::RemoveNode(Node* N) {
  auto It = std::find(Nodes.cbegin(), Nodes.cend(), N);
  if(It == Nodes.cend()) return false;
  Nodes.erase(It);
  return true;
}
