#ifndef VULKANCLASSES_PIPELINE_CACHE_H
#define VULKANCLASSES_PIPELINE_CACHE_H
#include "pipeline.h"

namespace vkc
{
	class PipelineCache final
	{
	public:
		struct Data final
		{
			explicit Data(std::vector<char>&& cache)
				: Cache{ std::move(cache) }
				, Header{ reinterpret_cast<VkPipelineCacheHeaderVersionOne const&>(*Cache.data()) } {}

			std::vector<char> const                Cache;
			VkPipelineCacheHeaderVersionOne const& Header;
		};

		PipelineCache() = delete;
		PipelineCache(Context const& context, std::span<char> initialData, VkPipelineCacheCreateFlagBits flags);
		~PipelineCache() = default;

		PipelineCache(PipelineCache&&)                 = delete;
		PipelineCache(PipelineCache const&)            = delete;
		PipelineCache& operator=(PipelineCache&&)      = delete;
		PipelineCache& operator=(PipelineCache const&) = delete;

		void Destroy(Context const& context) const
		{
			context.DispatchTable.destroyPipelineCache(m_PipelineCache, nullptr);
		}

		[[nodiscard]] Data AcquireCache(Context const& context) const
		{
			size_t cacheSize{};
			context.DispatchTable.getPipelineCacheData(m_PipelineCache, &cacheSize, nullptr);
			std::vector<char> cache;
			cache.resize(cacheSize);
			context.DispatchTable.getPipelineCacheData(m_PipelineCache, &cacheSize, cache.data());
			return Data{ std::move(cache) };
		}

		operator VkPipelineCache() const
		{
			return m_PipelineCache;
		}

		operator VkPipelineCache const*() const
		{
			return &m_PipelineCache;
		}

	private:
		VkPipelineCache m_PipelineCache{};
	};
}

#endif //VULKANCLASSES_PIPELINE_CACHE_H
