#include "UiRenderNode.h"

#include "Engine/Engine.h"

ORenderTargetBase* OUIRenderNode::Execute(ORenderTargetBase* RenderTarget)
{
	auto manager = OEngine::Get()->GetUIManager().lock();
	manager->Draw();
	manager->PostRender(CommandQueue->GetCommandList().Get());
	return RenderTarget;
}

void OUIRenderNode::SetupCommonResources()
{
}