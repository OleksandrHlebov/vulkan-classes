#include "pipeline_cache.h"

vkc::PipelineCache::PipelineCache(Context const& context, std::span<char> initialData, VkPipelineCacheCreateFlagBits flags)
{
	VkPipelineCacheCreateInfo pipelineCacheCreateInfo{};
	pipelineCacheCreateInfo.sType           = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
	pipelineCacheCreateInfo.flags           = flags;
	pipelineCacheCreateInfo.initialDataSize = initialData.size() * sizeof(initialData[0]);
	pipelineCacheCreateInfo.pInitialData    = initialData.data();
	context.DispatchTable.createPipelineCache(&pipelineCacheCreateInfo, nullptr, &m_PipelineCache);
}
