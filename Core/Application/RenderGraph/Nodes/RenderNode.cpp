#include "RenderNode.h"

#include "Engine/Engine.h"
#include "EngineHelper.h"

ORenderTargetBase* ORenderNode::Execute(ORenderTargetBase* RenderTarget)
{
	return RenderTarget;
}

void ORenderNode::SetupCommonResources()
{
}

void ORenderNode::SetPSO(const string& PSOType) const
{
	ParentGraph->SetPSO(PSOType);
}

SPSODescriptionBase* ORenderNode::FindPSOInfo(SPSOType Name) const
{
	return ParentGraph->FindPSOInfo(Name);
}

void ORenderNode::SetNodeEnabled(bool bEnable)
{
	NodeInfo.bEnable = bEnable;
}

void ORenderNode::Initialize(const SNodeInfo& OtherNodeInfo, OCommandQueue* OtherCommandQueue, ORenderGraph* OtherParentGraph, const SPSOType& Type)
{
	NodeInfo = OtherNodeInfo;
	CommandQueue = OtherCommandQueue;
	PSO = Type;
	ParentGraph = OtherParentGraph;
}
