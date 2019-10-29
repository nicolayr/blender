#include "BKE_multi_function_network.h"

namespace BKE {

/* MFNetwork Builder
 **************************************/

MFNetworkBuilder::~MFNetworkBuilder()
{
  for (auto node : m_function_nodes) {
    delete node;
  }
  for (auto node : m_placeholder_nodes) {
    delete node;
  }
  for (auto socket : m_input_sockets) {
    delete socket;
  }
  for (auto socket : m_output_sockets) {
    delete socket;
  }
}

MFBuilderFunctionNode &MFNetworkBuilder::add_function(const MultiFunction &function,
                                                      ArrayRef<uint> input_param_indices,
                                                      ArrayRef<uint> output_param_indices)
{
#ifdef DEBUG
  BLI_assert(!input_param_indices.has_duplicates__linear_search());
  BLI_assert(!output_param_indices.has_duplicates__linear_search());
  for (uint i = 0; i < function.signature().param_types().size(); i++) {
    BLI_assert(input_param_indices.contains(i) || output_param_indices.contains(i));
  }
#endif

  auto node = new MFBuilderFunctionNode();

  node->m_network = this;
  node->m_is_placeholder = false;
  node->m_function = &function;
  node->m_input_param_indices = input_param_indices;
  node->m_output_param_indices = output_param_indices;
  node->m_id = m_node_by_id.size();

  for (uint i = 0; i < input_param_indices.size(); i++) {
    uint param_index = input_param_indices[i];
    MFParamType param = function.signature().param_types()[param_index];
    BLI_assert(param.is_input_or_mutable());

    auto input_socket = new MFBuilderInputSocket();
    input_socket->m_type = param.as_data_type();
    input_socket->m_node = node;
    input_socket->m_index = i;
    input_socket->m_is_output = false;
    input_socket->m_id = m_socket_by_id.size();
    node->m_inputs.append(input_socket);
    m_socket_by_id.append(input_socket);
    m_input_sockets.append(input_socket);
  }

  for (uint i = 0; i < output_param_indices.size(); i++) {
    uint param_index = output_param_indices[i];
    MFParamType param = function.signature().param_types()[param_index];
    BLI_assert(param.is_output_or_mutable());

    auto output_socket = new MFBuilderOutputSocket();
    output_socket->m_type = param.as_data_type();
    output_socket->m_node = node;
    output_socket->m_index = i;
    output_socket->m_is_output = true;
    output_socket->m_id = m_socket_by_id.size();
    node->m_outputs.append(output_socket);
    m_socket_by_id.append(output_socket);
    m_output_sockets.append(output_socket);
  }

  m_function_nodes.append(node);
  m_node_by_id.append(node);
  return *node;
}

MFBuilderPlaceholderNode &MFNetworkBuilder::add_placeholder(ArrayRef<MFDataType> input_types,
                                                            ArrayRef<MFDataType> output_types)
{
  auto node = new MFBuilderPlaceholderNode();

  node->m_network = this;
  node->m_is_placeholder = true;
  node->m_id = m_node_by_id.size();

  for (uint i = 0; i < input_types.size(); i++) {
    auto input_socket = new MFBuilderInputSocket();
    input_socket->m_type = input_types[i];
    input_socket->m_node = node;
    input_socket->m_index = i;
    input_socket->m_is_output = false;
    input_socket->m_id = m_socket_by_id.size();
    node->m_inputs.append(input_socket);
    m_socket_by_id.append(input_socket);
    m_input_sockets.append(input_socket);
  }
  for (uint i = 0; i < output_types.size(); i++) {
    auto output_socket = new MFBuilderOutputSocket();
    output_socket->m_type = output_types[i];
    output_socket->m_node = node;
    output_socket->m_index = i;
    output_socket->m_is_output = true;
    output_socket->m_id = m_socket_by_id.size();
    node->m_outputs.append(output_socket);
    m_socket_by_id.append(output_socket);
    m_output_sockets.append(output_socket);
  }

  m_placeholder_nodes.append(node);
  m_node_by_id.append(node);
  return *node;
}

void MFNetworkBuilder::add_link(MFBuilderOutputSocket &from, MFBuilderInputSocket &to)
{
  BLI_assert(to.origin() == nullptr);
  BLI_assert(from.m_node->m_network == to.m_node->m_network);
  from.m_targets.append(&to);
  to.m_origin = &from;
}

/* Network
 ********************************************/

MFNetwork::MFNetwork(std::unique_ptr<MFNetworkBuilder> builder)
{
  m_node_by_id = Array<MFNode *>(builder->nodes_by_id().size());
  m_socket_by_id = Array<MFSocket *>(builder->sockets_by_id().size());

  for (MFBuilderFunctionNode *builder_node : builder->function_nodes()) {
    MFFunctionNode *node = new MFFunctionNode();

    node->m_function = &builder_node->function();
    node->m_id = builder_node->id();
    node->m_input_param_indices = builder_node->input_param_indices();
    node->m_output_param_indices = builder_node->output_param_indices();
    node->m_network = this;
    node->m_is_placeholder = false;

    for (MFBuilderInputSocket *builder_socket : builder_node->inputs()) {
      MFInputSocket *socket = new MFInputSocket();
      socket->m_id = builder_socket->id();
      socket->m_index = builder_socket->index();
      socket->m_is_output = false;
      socket->m_node = node;
      socket->m_type = builder_socket->type();

      m_socket_by_id[socket->id()] = socket;
      m_input_sockets.append(socket);
      node->m_inputs.append(socket);
    }
    for (MFBuilderOutputSocket *builder_socket : builder_node->outputs()) {
      MFOutputSocket *socket = new MFOutputSocket();
      socket->m_id = builder_socket->id();
      socket->m_index = builder_socket->index();
      socket->m_is_output = true;
      socket->m_node = node;
      socket->m_type = builder_socket->type();

      m_socket_by_id[socket->id()] = socket;
      m_output_sockets.append(socket);
      node->m_outputs.append(socket);
    }

    m_function_nodes.append(node);
    m_node_by_id[node->id()] = node;
  }

  for (MFBuilderPlaceholderNode *builder_node : builder->placeholder_nodes()) {
    MFPlaceholderNode *node = new MFPlaceholderNode();

    node->m_id = builder_node->id();
    node->m_network = this;
    node->m_is_placeholder = false;

    for (MFBuilderInputSocket *builder_socket : builder_node->inputs()) {
      MFInputSocket *socket = new MFInputSocket();
      socket->m_id = builder_socket->id();
      socket->m_index = builder_socket->index();
      socket->m_is_output = false;
      socket->m_node = node;
      socket->m_type = builder_socket->type();

      m_socket_by_id[socket->id()] = socket;
      m_input_sockets.append(socket);
      node->m_inputs.append(socket);
    }
    for (MFBuilderOutputSocket *builder_socket : builder_node->outputs()) {
      MFOutputSocket *socket = new MFOutputSocket();
      socket->m_id = builder_socket->id();
      socket->m_index = builder_socket->index();
      socket->m_is_output = true;
      socket->m_node = node;
      socket->m_type = builder_socket->type();

      m_socket_by_id[socket->id()] = socket;
      m_output_sockets.append(socket);
      node->m_outputs.append(socket);
    }

    m_placeholder_nodes.append(node);
    m_node_by_id[node->id()] = node;
  }

  for (MFBuilderInputSocket *builder_socket : builder->input_sockets()) {
    MFInputSocket &socket = m_socket_by_id[builder_socket->id()]->as_input();
    MFOutputSocket &origin = m_socket_by_id[builder_socket->origin()->id()]->as_output();

    socket.m_origin = &origin;
  }

  for (MFBuilderOutputSocket *builder_socket : builder->output_sockets()) {
    MFOutputSocket &socket = m_socket_by_id[builder_socket->id()]->as_output();

    for (MFBuilderInputSocket *builder_target : builder_socket->targets()) {
      MFInputSocket &target = m_socket_by_id[builder_target->id()]->as_input();
      socket.m_targets.append(&target);
    }
  }
}

MFNetwork::~MFNetwork()
{
  for (auto node : m_function_nodes) {
    delete node;
  }
  for (auto node : m_placeholder_nodes) {
    delete node;
  }
  for (auto socket : m_input_sockets) {
    delete socket;
  }
  for (auto socket : m_output_sockets) {
    delete socket;
  }
}

}  // namespace BKE