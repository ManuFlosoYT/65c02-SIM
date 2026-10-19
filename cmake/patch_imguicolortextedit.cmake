if(EXISTS "TextEditor.h")
    file(READ "TextEditor.h" HEADER_CONTENT)
    if(NOT HEADER_CONTENT MATCHES "TooltipCallback")
        string(REPLACE "#include <regex>" "#include <regex>\n#include <functional>" HEADER_CONTENT "${HEADER_CONTENT}")
        string(REPLACE "void SetBreakpoints(const Breakpoints& aMarkers) { mBreakpoints = aMarkers; }"
                       "void SetBreakpoints(const Breakpoints& aMarkers) { mBreakpoints = aMarkers; }\n\n\ttypedef std::function<std::string(const std::string&)> TooltipCallback;\n\tvoid SetTooltipCallback(TooltipCallback aCallback) { mTooltipCallback = std::move(aCallback); }"
                       HEADER_CONTENT "${HEADER_CONTENT}")
        string(REPLACE "Coordinates mInteractiveStart, mInteractiveEnd;"
                       "Coordinates mInteractiveStart, mInteractiveEnd;\n\tTooltipCallback mTooltipCallback;"
                       HEADER_CONTENT "${HEADER_CONTENT}")
        file(WRITE "TextEditor.h" "${HEADER_CONTENT}")
    endif()
endif()

if(EXISTS "TextEditor.cpp")
    file(READ "TextEditor.cpp" CPP_CONTENT)
    if(NOT CPP_CONTENT MATCHES "mTooltipCallback")
        set(SEARCH_STR "				auto it = mLanguageDefinition.mIdentifiers.find(id);
				if (it != mLanguageDefinition.mIdentifiers.end())
				{
					ImGui::BeginTooltip();
					ImGui::TextUnformatted(it->second.mDeclaration.c_str());
					ImGui::EndTooltip();
				}
				else
				{
					auto pi = mLanguageDefinition.mPreprocIdentifiers.find(id);
					if (pi != mLanguageDefinition.mPreprocIdentifiers.end())
					{
						ImGui::BeginTooltip();
						ImGui::TextUnformatted(pi->second.mDeclaration.c_str());
						ImGui::EndTooltip();
					}
				}")
        set(REPLACE_STR "				bool handled = false;
				if (mTooltipCallback)
				{
					std::string tip = mTooltipCallback(id);
					if (!tip.empty())
					{
						ImGui::BeginTooltip();
						ImGui::TextUnformatted(tip.c_str());
						ImGui::EndTooltip();
						handled = true;
					}
				}
				if (!handled)
				{
					auto it = mLanguageDefinition.mIdentifiers.find(id);
					if (it != mLanguageDefinition.mIdentifiers.end())
					{
						ImGui::BeginTooltip();
						ImGui::TextUnformatted(it->second.mDeclaration.c_str());
						ImGui::EndTooltip();
					}
					else
					{
						auto pi = mLanguageDefinition.mPreprocIdentifiers.find(id);
						if (pi != mLanguageDefinition.mPreprocIdentifiers.end())
						{
							ImGui::BeginTooltip();
							ImGui::TextUnformatted(pi->second.mDeclaration.c_str());
							ImGui::EndTooltip();
						}
					}
				}")
        string(REPLACE "${SEARCH_STR}" "${REPLACE_STR}" CPP_CONTENT "${CPP_CONTENT}")
        file(WRITE "TextEditor.cpp" "${CPP_CONTENT}")
    endif()
endif()

