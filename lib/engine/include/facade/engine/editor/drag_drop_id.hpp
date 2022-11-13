#pragma once
#include <imgui.h>
#include <facade/engine/editor/common.hpp>
#include <facade/scene/node.hpp>
#include <facade/util/fixed_string.hpp>
#include <optional>
#include <typeinfo>

namespace facade::editor {
///
/// \brief Initiate dragging an Id<T> drag-drop payload.
/// \param out Id to store as the drag-drop payload
/// \param label Label to display on drag
/// \returns true if dragging is engaged
///
template <typename T>
bool drag_payload(Id<T> id, char const* label) {
	if (ImGui::BeginDragDropSource()) {
		ImGui::SetDragDropPayload(typeid(id).name(), &id, sizeof(id));
		if (label && *label) { ImGui::Text("%s", label); }
		ImGui::EndDragDropSource();
		return true;
	}
	return false;
}

///
/// \brief Accept dropping an Id<T> drag-drop payload.
/// \param out Id to write payload to
/// \returns true if payload accepted
///
template <typename T>
bool accept_drop(Id<T>& out) {
	if (ImGui::BeginDragDropTarget()) {
		if (ImGuiPayload const* payload = ImGui::AcceptDragDropPayload(typeid(out).name())) {
			assert(payload->DataSize == sizeof(out));
			out = *reinterpret_cast<Id<T>*>(payload->Data);
			return true;
		}
		ImGui::EndDragDropTarget();
	}
	return false;
}

///
/// \brief Create a potentially nullable drag-drop Id slot.
/// \param out_id Id to use for drag-drop payload
/// \param label Label to use for the slot
/// \param payload_name Label to use when dragging payload
/// \param removable Whether to allow nulling out out_id
///
template <typename T>
void make_id_slot(std::optional<Id<T>>& out_id, char const* label, char const* payload_name, Bool removable) {
	ImGui::Text("%s", label);
	if (out_id) {
		if (removable) {
			ImGui::SameLine();
			if (small_button_red(FixedString{"x###remove_{}", label}.c_str())) {
				out_id = {};
				return;
			}
		}
		ImGui::SameLine();
		auto const name = FixedString{"{} ({})", payload_name, *out_id};
		ImGui::Selectable(name.c_str());
		drag_payload(*out_id, name.c_str());
		accept_drop(*out_id);
	} else {
		ImGui::SameLine();
		ImGui::Selectable("[None]");
		auto id = Id<T>{};
		if (accept_drop(id)) { out_id = id; }
	}
}

///
/// \brief Create a drag-drop Id slot.
/// \param out_id Id to use for drag-drop payload
/// \param label Label to use for the slot
/// \param payload_name Label to use when dragging payload
///
template <typename T>
void make_id_slot(Id<T>& out_id, char const* label, char const* payload_name) {
	auto id = std::optional<Id<T>>{out_id};
	make_id_slot(id, label, payload_name, {false});
	out_id = *id;
}

///
/// \brief Create a drag-drop Id slot.
/// \param out_node Node to attach / detach Id<T> drag-drop payloads to / from
/// \param label Label to use for the slot
/// \param payload_name Label to use when dragging payload
///
template <typename T>
void make_id_slot(Node& out_node, char const* label, char const* payload_name) {
	auto oid = std::optional<Id<T>>{};
	auto* id = out_node.find<Id<T>>();
	if (id) { oid = *id; }
	make_id_slot(oid, label, payload_name, {true});
	if (id) {
		if (!oid) {
			out_node.detach<Id<T>>();
		} else {
			*id = *oid;
		}
	} else {
		if (oid) { out_node.attach(*oid); }
	}
}
} // namespace facade::editor
