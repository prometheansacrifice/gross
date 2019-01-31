#ifndef GROSS_GRAPH_BGL_H
#define GROSS_GRAPH_BGL_H
/// Defining required traits for boost graph library
#include "boost/graph/graph_traits.hpp"
#include "boost/graph/properties.hpp"
#include "boost/property_map/property_map.hpp"
#include "boost/iterator/transform_iterator.hpp"
#include "gross/Graph/Graph.h"
#include "gross/Graph/Node.h"
#include "gross/Graph/NodeUtils.h"
#include "gross/Support/STLExtras.h"
#include <utility>
#include <iostream>

namespace gross {
// since boost::depth_first_search has some really STUPID
// copy by value ColorMap parameter, we need some stub/proxy
// to hold map storage across several usages.
template<class T>
struct StubColorMap {
  using value_type = boost::default_color_type;
  using reference = value_type&;
  using key_type = Node*;
  struct category : public boost::read_write_property_map_tag {};

  StubColorMap(T& Impl) : Storage(Impl) {}
  StubColorMap() = delete;
  StubColorMap(const StubColorMap& Other) = default;

  reference get(const key_type& key) const {
    return const_cast<reference>(Storage.at(key));
  }
  void put(const key_type& key, const value_type& val) {
    Storage[key] = val;
  }

private:
  T& Storage;
};

// it is very strange that both StubColorMap and Graph::id_map<T>
// are PropertyMapConcept, but the get() function for the former one
// should be defined in namespace gross :\
/// ColorMap PropertyMap
template<class T>
inline typename gross::StubColorMap<T>::reference
get(const gross::StubColorMap<T>& pmap,
    const typename gross::StubColorMap<T>::key_type& key) {
  return pmap.get(key);
}

template<class T> inline
void put(gross::StubColorMap<T>& pmap,
         const typename gross::StubColorMap<T>::key_type& key,
         const typename gross::StubColorMap<T>::value_type& val) {
  return pmap.put(key, val);
}
} // end namespace gross

namespace boost {
template<>
struct graph_traits<gross::Graph> {
  /// GraphConcept
  using vertex_descriptor = gross::Node*;
  using edge_descriptor = gross::Use;
  using directed_category = boost::directed_tag;
  using edge_parallel_category = boost::allow_parallel_edge_tag;

  static vertex_descriptor null_vertex() {
    return nullptr;
  }

  /// VertexListGraphConcept
  using vertex_iterator
    = boost::transform_iterator<gross::unique_ptr_unwrapper<gross::Node>,
                                typename gross::Graph::node_iterator,
                                gross::Node*, // Refrence type
                                gross::Node* // Value type
                                >;
  using vertices_size_type = size_t;

  /// EdgeListGraphConcept
  using edges_size_type = size_t;
  using edge_iterator = typename gross::Graph::edge_iterator;

  /// IncidenceGraphConcept
  using out_edge_iterator
    = boost::transform_iterator<typename gross::Use::BuilderFunctor,
                                typename gross::Node::input_iterator,
                                gross::Use, // Reference type
                                gross::Use // Value type
                                >;
  using degree_size_type = size_t;

  struct traversal_category :
    public boost::vertex_list_graph_tag,
    public boost::edge_list_graph_tag,
    public boost::incidence_graph_tag {};
};

/// Note: We mark most of the BGL trait functions here as inline
/// because they're trivial.
/// FIXME: Will putting them into separated source file helps reducing
/// compilation time?

/// VertexListGraphConcept
inline
std::pair<typename boost::graph_traits<gross::Graph>::vertex_iterator,
          typename boost::graph_traits<gross::Graph>::vertex_iterator>
vertices(gross::Graph& g) {
  using vertex_it_t
    = typename boost::graph_traits<gross::Graph>::vertex_iterator;
  gross::unique_ptr_unwrapper<gross::Node> functor;
  return std::make_pair(
    vertex_it_t(g.node_begin(), functor),
    vertex_it_t(g.node_end(), functor)
  );
}
inline
std::pair<typename boost::graph_traits<gross::Graph>::vertex_iterator,
          typename boost::graph_traits<gross::Graph>::vertex_iterator>
vertices(const gross::Graph& g) {
  return vertices(const_cast<gross::Graph&>(g));
}

inline typename boost::graph_traits<gross::Graph>::vertices_size_type
num_vertices(gross::Graph& g) {
  return const_cast<const gross::Graph&>(g).node_size();
}
inline typename boost::graph_traits<gross::Graph>::vertices_size_type
num_vertices(const gross::Graph& g) {
  return g.node_size();
}

/// EdgeListGraphConcept
inline
std::pair<typename boost::graph_traits<gross::Graph>::edge_iterator,
          typename boost::graph_traits<gross::Graph>::edge_iterator>
edges(gross::Graph& g) {
  return std::make_pair(g.edge_begin(), g.edge_end());
}
inline
std::pair<typename boost::graph_traits<gross::Graph>::edge_iterator,
          typename boost::graph_traits<gross::Graph>::edge_iterator>
edges(const gross::Graph& g) {
  return edges(const_cast<gross::Graph&>(g));
}

inline typename boost::graph_traits<gross::Graph>::edges_size_type
num_edges(gross::Graph& g) {
  return const_cast<const gross::Graph&>(g).edge_size();
}
inline typename boost::graph_traits<gross::Graph>::edges_size_type
num_edges(const gross::Graph& g) {
  return g.edge_size();
}

inline typename boost::graph_traits<gross::Graph>::vertex_descriptor
source(const gross::Use& e, const gross::Graph& g) {
  return const_cast<gross::Node*>(e.Source);
}
inline typename boost::graph_traits<gross::Graph>::vertex_descriptor
target(const gross::Use& e, const gross::Graph& g) {
  return const_cast<gross::Node*>(e.Dest);
}

/// IncidenceGraphConcept
inline
std::pair<typename boost::graph_traits<gross::Graph>::out_edge_iterator,
          typename boost::graph_traits<gross::Graph>::out_edge_iterator>
out_edges(gross::Node* u, const gross::Graph& g) {
  // for now, we don't care about the kind of edge
  using edge_it_t
    = typename boost::graph_traits<gross::Graph>::out_edge_iterator;
  gross::Use::BuilderFunctor functor(u);
  return std::make_pair(
    edge_it_t(u->inputs().begin(), functor),
    edge_it_t(u->inputs().end(), functor)
  );
}

inline
typename boost::graph_traits<gross::Graph>::degree_size_type
out_degree(gross::Node* u, const gross::Graph& g) {
  return u->getNumValueInput()
         + u->getNumControlInput()
         + u->getNumEffectInput();
}
} // end namespace boost

/// Property Map Concept
namespace gross {
template<>
struct Graph::id_map<boost::vertex_index_t> {
  using value_type = size_t;
  using reference = size_t;
  using key_type = Node*;
  struct category : public boost::readable_property_map_tag {};

  id_map(const Graph& g) : G(g) {}

  reference operator[](const key_type& key) const {
    // just use linear search for now
    // TODO: improve time complexity
    value_type Idx = 0;
    for(auto I = G.node_cbegin(), E = G.node_cend();
        I != E; ++I, ++Idx) {
      if(I->get() == key) break;
    }
    return Idx;
  }

private:
  const Graph &G;
};
} // end namespace gross

namespace boost {
// get() for vertex id property map
inline typename gross::Graph::id_map<boost::vertex_index_t>::reference
get(const typename gross::Graph::id_map<boost::vertex_index_t> &pmap,
    const typename gross::Graph::id_map<boost::vertex_index_t>::key_type &key) {
  return pmap[key];
}
// get() for getting vertex id property map from graph
inline typename gross::Graph::id_map<boost::vertex_index_t>
get(boost::vertex_index_t tag, const gross::Graph& g) {
  return typename gross::Graph::id_map<boost::vertex_index_t>(g);
}
} // end namespace boost

/// PropertyWriter Concept
namespace gross {
struct graph_prop_writer {
  void operator()(std::ostream& OS) const {
    // print the graph 'upside down'
    OS << "rankdir = BT;" << std::endl;
  }
};

struct graph_vertex_prop_writer {
  graph_vertex_prop_writer(const Graph& g) : G(g) {}

  void operator()(std::ostream& OS, const Node* v) const {
    Node* N = const_cast<Node*>(v);
#define CASE_OPCODE_STR(OC)  \
    case IrOpcode::OC:    \
      OS << "[label=\""   \
         << #OC           \
         << "\"]";        \
      break

    switch(N->getOp()) {
    case IrOpcode::ConstantInt: {
      OS << "[label=\"ConstInt<"
         << NodeProperties<IrOpcode::ConstantInt>(N).as<int32_t>(G)
         << ">\"]";
      break;
    }
    case IrOpcode::ConstantStr: {
      OS << "[label=\"ConstStr<"
         << NodeProperties<IrOpcode::ConstantStr>(N).str(G)
         << ">\"]";
      break;
    }
    CASE_OPCODE_STR(BinAdd);
    CASE_OPCODE_STR(BinSub);
    CASE_OPCODE_STR(BinMul);
    CASE_OPCODE_STR(BinDiv);
    CASE_OPCODE_STR(BinGe);
    CASE_OPCODE_STR(BinGt);
    CASE_OPCODE_STR(BinLe);
    CASE_OPCODE_STR(BinLt);
    CASE_OPCODE_STR(BinNe);
    CASE_OPCODE_STR(BinEq);
    CASE_OPCODE_STR(SrcVarDecl);
    CASE_OPCODE_STR(SrcVarAccess);
    CASE_OPCODE_STR(SrcArrayDecl);
    CASE_OPCODE_STR(SrcArrayAccess);
    CASE_OPCODE_STR(SrcAssignStmt);
    CASE_OPCODE_STR(If);
    CASE_OPCODE_STR(IfTrue);
    CASE_OPCODE_STR(IfFalse);
    CASE_OPCODE_STR(Merge);
    CASE_OPCODE_STR(Phi);
    CASE_OPCODE_STR(Start);
    CASE_OPCODE_STR(End);
    CASE_OPCODE_STR(Argument);
    CASE_OPCODE_STR(Return);
    default: OS << "UNKNOWN";
    }

#undef CASE_OPCODE_STR
  }

private:
  const Graph& G;
};

struct graph_edge_prop_writer {
  void operator()(std::ostream& OS, const Use& U) const {
    switch(U.DepKind) {
    case Use::K_VALUE:
      OS << "[color=\"black\"]";
      break;
    case Use::K_CONTROL:
      OS << "[color=\"blue\"]";
      break;
    case Use::K_EFFECT:
      OS << "[color=\"red\", style=\"dashed\"]";
      break;
    default:
      gross_unreachable("Invalid edge kind");
    }
  }
};
} // end namespace gross
#endif
