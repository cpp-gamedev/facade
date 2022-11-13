#pragma once
#include <imgui.h>
#include <facade/engine/editor/common.hpp>
#include <facade/scene/node.hpp>
#include <facade/util/fixed_string.hpp>
#include <optional>

namespace facade::editor {
template <typename T>
bool drag_payload(Id<T> out, char const* type, char const* label) {
	if (ImGui::BeginDragDropSource()) {
		ImGui::SetDragDropPayload(type, &out, sizeof(out));
		ImGui::Text("%s", label);
		ImGui::EndDragDropSource();
		return true;
	}
	return false;
}

template <typename T>
bool accept_drop(Id<T>& out, char const* type) {
	if (ImGui::BeginDragDropTarget()) {
		if (ImGuiPayload const* payload = ImGui::AcceptDragDropPayload(type)) {
			assert(payload->DataSize == sizeof(Id<T>));
			out = *reinterpret_cast<Id<T>*>(payload->Data);
			return true;
		}
		ImGui::EndDragDropTarget();
	}
	return false;
}

template <typename T>
void make_id_slot(std::optional<Id<T>>& out_id, char const* label, char const* payload_name, char const* payload_type, Bool removable) {
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
		drag_payload(*out_id, payload_type, name.c_str());
		accept_drop(*out_id, payload_type);
	} else {
		ImGui::SameLine();
		ImGui::Selectable("[None]");
		auto id = Id<T>{};
		if (accept_drop(id, payload_type)) { out_id = id; }
	}
}

template <typename T>
void make_id_slot(Id<T>& out_id, char const* label, char const* payload_name, char const* payload_type) {
	auto id = std::optional<Id<T>>{out_id};
	make_id_slot(id, label, payload_name, payload_type, {false});
	out_id = *id;
}

template <typename T>
void make_id_slot(Node& out_node, char const* label, char const* payload_name, char const* payload_type) {
	auto oid = std::optional<Id<T>>{};
	auto* id = out_node.find<Id<T>>();
	if (id) { oid = *id; }
	make_id_slot(oid, label, payload_name, payload_type, {true});
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
