#ifndef DESCRIPTOR_SET_H
#define DESCRIPTOR_SET_H

#include <span>

#include "context.h"

namespace vkc
{
	class DescriptorSet final
	{
	public:
		~DescriptorSet() = default;

		DescriptorSet(DescriptorSet&&)                 = default;
		DescriptorSet(DescriptorSet const&)            = delete;
		DescriptorSet& operator=(DescriptorSet&&)      = default;
		DescriptorSet& operator=(DescriptorSet const&) = delete;

		DescriptorSet& AddWriteDescriptor
		(std::span<VkDescriptorImageInfo> imageInfos, VkDescriptorType type, uint32_t binding, uint32_t arrayElement);

		DescriptorSet& AddWriteDescriptor
		(std::span<VkDescriptorBufferInfo> bufferInfos, VkDescriptorType type, uint32_t binding, uint32_t arrayElement);

		void Update(Context const& context);

		operator VkDescriptorSet() const
		{
			return m_Set;
		}

		operator VkDescriptorSet*()
		{
			return &m_Set;
		}

	private:
		friend class DescriptorSetBuilder;

		DescriptorSet(VkDescriptorSet set)
			: m_Set{ set } {}

		VkDescriptorSet m_Set;

		std::vector<VkWriteDescriptorSet> m_WriteDescriptorSets{};
	};

	class DescriptorSetBuilder final
	{
	public:
		DescriptorSetBuilder() = delete;

		DescriptorSetBuilder(Context& context);

		~DescriptorSetBuilder() = default;

		DescriptorSetBuilder(DescriptorSetBuilder&&)                 = delete;
		DescriptorSetBuilder(DescriptorSetBuilder const&)            = delete;
		DescriptorSetBuilder& operator=(DescriptorSetBuilder&&)      = delete;
		DescriptorSetBuilder& operator=(DescriptorSetBuilder const&) = delete;

		DescriptorSetBuilder&                    AddVariableDescriptorCount(std::span<uint32_t> counts);
		[[nodiscard]] std::vector<DescriptorSet> Build(VkDescriptorPool pool, std::span<VkDescriptorSetLayout> layouts) const;

	private:
		Context&                                           m_Context;
		VkDescriptorSetVariableDescriptorCountAllocateInfo m_DescriptorCountAllocInfo{
			VK_STRUCTURE_TYPE_DESCRIPTOR_SET_VARIABLE_DESCRIPTOR_COUNT_ALLOCATE_INFO
			, nullptr
			, 0
			, nullptr
		};
	};
}

#endif //DESCRIPTOR_SET_H
