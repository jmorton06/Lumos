// for the license, see the end of the file
#pragma once

#include <set>
#include <map>

#include <entt/entt.hpp>
#include <imgui.h>

// if you have font awesome or something comparable you can set this to a wastebin
#ifndef ESS_IMGUI_ENTT_E_E_DELETE_COMP_STR
	#define ESS_IMGUI_ENTT_E_E_DELETE_COMP_STR "-"
#endif

namespace MM {

template<typename Registry>
class ImGuiEntityEditor {
	private:
		using component_type = entt::component;

		std::set<component_type> _component_types;
		std::map<component_type, std::string> _component_names;
		std::map<component_type, void(*)(Registry&, typename Registry::entity_type)> _component_widget;
		std::map<component_type, void(*)(Registry&, typename Registry::entity_type)> _component_create;
		std::map<component_type, void(*)(Registry&, typename Registry::entity_type)> _component_destroy;

	public:
		bool show_window = true;

	private:
		inline bool entity_has_component(Registry& ecs, typename Registry::entity_type& e, component_type ct) {
			component_type type[] = { ct };
			auto rv = ecs.runtime_view(std::cbegin(type), std::cend(type));
			return rv.contains(e);
		}

	public:
		// calls all the ImGui functions
		// call this every frame
		void renderImGui(Registry& ecs, typename Registry::entity_type& e) {
			if (show_window) {
				if(ImGui::Begin("Entity Editor", &show_window)) {
					ImGui::TextUnformatted("editing:");
					ImGui::SameLine();

					//ImGuiWidgets::Entity(e, ecs, true);
					if (ecs.valid(e)) {
						ImGui::Text("id: %d, v: %d", ecs.entity(e), ecs.version(e));
					} else {
						ImGui::Text("INVALID ENTITY");
					}
					// TODO: investigate

					if (ImGui::Button("New Entity")) {
						e = ecs.create();
					}

					// TODO: implemnt cloning by ether forking entt or implementing function lists...
					//ImGui::SameLine();
					//ImGui::TextUnformatted(ICON_II_ARCHIVE " drop to clone Entity");
					//if (ImGui::BeginDragDropTarget()) {
						//if (auto* payload = ImGui::AcceptDragDropPayload(IMGUI_PAYLOAD_TYPE_MM_ENTITY)) {
							//auto clone_e = *(MM::FrameworkConfig::Entity*)payload->Data;
							//e = ecs.clone(clone_e);
						//}
						//ImGui::EndDragDropTarget();
					//}

					ImGui::Separator();

					// TODO: needed?
					if (!ecs.valid(e)) {
						e = entt::null;
					}

					if (e != entt::null) {
						std::vector<component_type> has_not;
						for (auto ct : _component_types) {
							if (entity_has_component(ecs, e, ct)) {

								// delete component button
								if (_component_destroy.count(ct)) {
									std::string button_label = ESS_IMGUI_ENTT_E_E_DELETE_COMP_STR "##";
									button_label += entt::to_integer(ct);

									if (ImGui::Button(button_label.c_str())) {
										_component_destroy[ct](ecs, e);
										continue; // early out to prevent access to deleted data
									} else {
										ImGui::SameLine();
									}
								}

								std::string label;
								if (_component_names.count(ct)) {
									label = _component_names[ct];
								} else {
									label = "unnamed component (";
									label += entt::to_integer(ct);
									label += ")";
								}

								if (ImGui::CollapsingHeader(label.c_str())) {
									ImGui::Indent(30.f);

									if (_component_widget.count(ct)) {
										_component_widget[ct](ecs, e);
									} else {
										ImGui::TextDisabled("missing widget to display component!");
									}

									ImGui::Unindent(30.f);
								}
							} else {
								has_not.push_back(ct);
							}
						}

						if (!has_not.empty()) {
							if (ImGui::Button("+ Add Component")) {
								ImGui::OpenPopup("add component");
							}

							if (ImGui::BeginPopup("add component")) {
								ImGui::TextUnformatted("available:");
								ImGui::Separator();

								for (auto ct : has_not) {
									if (_component_create.count(ct)) {
										std::string label;
										if (_component_names.count(ct)) {
											label = _component_names[ct];
										} else {
											label = "unnamed component (";
											label += entt::to_integer(ct);
											label += ")";
										}

										label += "##"; label += entt::to_integer(ct); // better but optional

										if (ImGui::Selectable(label.c_str())) {
											_component_create[ct](ecs, e);
										}
									}
								}

								ImGui::EndPopup();
							}
						}
					}
				}
				ImGui::End();
			}
		}

		// call this (or registerTrivial) before any of the other register functions
		void registerComponentType(component_type ct) {
			if (!_component_types.count(ct)) {
				_component_types.emplace(ct);
			}
		}

		// register a name to be displayed for the component
		void registerComponentName(component_type ct, const std::string& name) {
			_component_names[ct] = name;
		}

		// register a callback to a function displaying a component. using imgui
		void registerComponentWidgetFn(component_type ct, void(*fn)(Registry&, typename Registry::entity_type)) {
			_component_widget[ct] = fn;
		}

		// register a callback to create a component, if none, you wont be able to create it in the editor
		void registerComponentCreateFn(component_type ct, void(*fn)(Registry&, typename Registry::entity_type)) {
			_component_create[ct] = fn;
		}

		// register a callback to delete a component, if none, you wont be able to delete it in the editor
		void registerComponentDestroyFn(component_type ct, void(*fn)(Registry&, typename Registry::entity_type)) {
			_component_destroy[ct] = fn;
		}

		// registers the component_type, name, create and destroy for rather trivial types
		template<typename T>
		void registerTrivial(Registry& ecs, const std::string& name) {
			registerComponentType(ecs.template type<T>());
			registerComponentName(ecs.template type<T>(), name);
			registerComponentCreateFn(ecs.template type<T>(),
				[](Registry& ecs, typename Registry::entity_type e) {
					ecs.template assign<T>(e);
				});
			registerComponentDestroyFn(ecs.template type<T>(),
				[](Registry& ecs, typename Registry::entity_type e) {
					ecs.template remove<T>(e);
				});
		}
};

} // MM

// MIT License

// Copyright (c) 2019 Erik Scholz

// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:

// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

