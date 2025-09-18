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
				, Header{
					[this]
					{
						if (Cache.size() < sizeof(Header))
							throw std::runtime_error("Pipeline cache too small for header");
						VkPipelineCacheHeaderVersionOne result{};
						std::memcpy(&result, Cache.data(), sizeof(Header));
						return result;
					}()
				} {}

			std::vector<char> const               Cache;
			VkPipelineCacheHeaderVersionOne const Header;
		};

		PipelineCache() = delete;

		template<typename DataType>
		PipelineCache(Context const& context, std::span<DataType> initialData, VkPipelineCacheCreateFlagBits flags)
		{
			VkPipelineCacheCreateInfo pipelineCacheCreateInfo{};
			pipelineCacheCreateInfo.sType           = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
			pipelineCacheCreateInfo.flags           = flags;
			pipelineCacheCreateInfo.initialDataSize = initialData.size() * sizeof(initialData[0]);
			pipelineCacheCreateInfo.pInitialData    = initialData.data();
			context.DispatchTable.createPipelineCache(&pipelineCacheCreateInfo, nullptr, &m_PipelineCache);
		}

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
