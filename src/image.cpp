#include "image.h"

vkc::ImageView vkc::Image::CreateView
(
	Context&          context
	, VkImageViewType type
	, uint32_t        baseLayer
	, uint32_t        layerCount
	, uint32_t        baseMipLevel
	, uint32_t        levelCount
	, bool
	addToQueue
) const
{
	ImageView imageView{};

	VkImageViewCreateInfo imageViewCreateInfo{};
	imageViewCreateInfo.sType                           = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	imageViewCreateInfo.flags                           = 0;
	imageViewCreateInfo.image                           = *this;
	imageViewCreateInfo.viewType                        = imageView.m_Type = type;
	imageViewCreateInfo.format                          = m_Format;
	imageViewCreateInfo.subresourceRange.aspectMask     = m_AspectFlags;
	imageViewCreateInfo.subresourceRange.layerCount     = imageView.m_LayerCount    = layerCount;
	imageViewCreateInfo.subresourceRange.levelCount     = imageView.m_MipLevelCount = levelCount;
	imageViewCreateInfo.subresourceRange.baseMipLevel   = imageView.m_BaseMipLevel  = baseMipLevel;
	imageViewCreateInfo.subresourceRange.baseArrayLayer = imageView.m_BaseLayer     = baseLayer;

	if (auto const result = context.DispatchTable.createImageView(&imageViewCreateInfo, nullptr, imageView);
		result != VK_SUCCESS)
		throw std::runtime_error("Failed to create image view");

	if (addToQueue)
		context.DeletionQueue.Push([context = &context, imageView = imageView.m_ImageView]
		{
			context->DispatchTable.destroyImageView(imageView, nullptr);
		});

	return imageView;
}

void vkc::Image::Destroy(Context const& context) const
{
	vmaDestroyImage(context.Allocator, *this, m_Allocation);
}

void vkc::Image::MakeTransition(Context const& context, VkCommandBuffer commandBuffer, Transition const& transition)
{
	bool hasEqualLayouts{ true };

	VkImageLayout const baseLayout = GetLayout(transition.BaseLayer, transition.BaseMipLevel);

	for (uint32_t layer{ transition.BaseLayer }; layer < transition.BaseLayer + transition.LayerCount && hasEqualLayouts; ++layer)
		for (uint32_t mipLevel{ transition.BaseMipLevel }; mipLevel < transition.BaseMipLevel + transition.LevelCount; ++mipLevel)
			if (baseLayout != GetLayout(layer, mipLevel))
			{
				hasEqualLayouts = false;
				break;
			}

	std::vector const barriers
	{
		hasEqualLayouts
		? MakeBarriersForEqualLayouts(transition)
		: MakeBarriersForDifferentLayouts(transition)
	};

	VkDependencyInfo dependencyInfo{};
	dependencyInfo.sType                   = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
	dependencyInfo.pNext                   = nullptr;
	dependencyInfo.imageMemoryBarrierCount = static_cast<uint32_t>(barriers.size());
	dependencyInfo.pImageMemoryBarriers    = barriers.data();
	context.DispatchTable.cmdPipelineBarrier2(commandBuffer, &dependencyInfo);

	for (uint32_t layer{}; layer < transition.LayerCount; ++layer)
		for (uint32_t mipLevel{}; mipLevel < transition.LevelCount; ++mipLevel)
			m_Layouts[transition.BaseLayer + layer][transition.BaseMipLevel + mipLevel] = transition.NewLayout;
}

void vkc::Image::ConvertFromSwapchainVkImages(Context& context, std::vector<Image>& convertedImages)
{
	convertedImages.clear();
	convertedImages.reserve(context.Swapchain.image_count);
	auto const resultImages = context.Swapchain.get_images();
	if (!resultImages)
		throw std::runtime_error("Failed to get swapchain images " + resultImages.error().message());
	for (VkImage const& image: resultImages.value())
	{
		Image convertedImage{};
		convertedImage.m_Image       = image;
		convertedImage.m_Extent      = context.Swapchain.extent;
		convertedImage.m_Format      = context.Swapchain.image_format;
		convertedImage.m_AspectFlags = VK_IMAGE_ASPECT_COLOR_BIT;
		convertedImage.m_MipLevels   = 1;
		convertedImage.m_Layers      = 1;
		convertedImage.m_Layouts.resize(1);
		convertedImage.m_Layouts[0].resize(1);
		convertedImages.emplace_back(std::move(convertedImage));
	}
}

std::vector<VkImageMemoryBarrier2> vkc::Image::MakeBarriersForEqualLayouts(Transition const& transition) const
{
	VkImageMemoryBarrier2 memoryBarrier{};
	memoryBarrier.sType         = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
	memoryBarrier.image         = m_Image;
	memoryBarrier.srcAccessMask = transition.SrcAccessMask;
	memoryBarrier.dstAccessMask = transition.DstAccessMask;
	memoryBarrier.srcStageMask  = transition.SrcStageMask;
	memoryBarrier.dstStageMask  = transition.DstStageMask;
	memoryBarrier.oldLayout     = GetLayout(transition.BaseLayer, transition.BaseMipLevel);
	memoryBarrier.newLayout     = transition.NewLayout;

	memoryBarrier.subresourceRange.aspectMask     = m_AspectFlags;
	memoryBarrier.subresourceRange.layerCount     = transition.LayerCount;
	memoryBarrier.subresourceRange.baseArrayLayer = transition.BaseLayer;
	memoryBarrier.subresourceRange.levelCount     = transition.LevelCount;
	memoryBarrier.subresourceRange.baseMipLevel   = transition.BaseMipLevel;

	memoryBarrier.srcQueueFamilyIndex = transition.SrcQueue;
	memoryBarrier.dstQueueFamilyIndex = transition.DstQueue;

	return { memoryBarrier };
}

std::vector<VkImageMemoryBarrier2> vkc::Image::MakeBarriersForDifferentLayouts(Transition const& transition) const
{
	// could make it find sequential equal layouts to reduce the number of barriers used for performance

	std::vector<VkImageMemoryBarrier2> barriers{};
	for (uint32_t layer{ transition.BaseLayer }; layer < transition.LayerCount; ++layer)
		for (uint32_t mipLevel{ transition.BaseMipLevel }; mipLevel < transition.LevelCount; ++mipLevel)
			barriers.emplace_back(VkImageMemoryBarrier2{
									  .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2
									  , .pNext = nullptr
									  , .srcStageMask = transition.SrcStageMask
									  , .srcAccessMask = transition.SrcAccessMask
									  , .dstStageMask = transition.DstStageMask
									  , .dstAccessMask = transition.DstAccessMask
									  , .oldLayout = GetLayout(layer, mipLevel)
									  , .newLayout = transition.NewLayout
									  , .srcQueueFamilyIndex = transition.SrcQueue
									  , .dstQueueFamilyIndex = transition.DstQueue
									  , .image = m_Image
									  , .subresourceRange
									  {
										  m_AspectFlags
										  , mipLevel
										  , 1
										  , layer
										  , 1
									  }
								  });

	return barriers;
}

vkc::ImageBuilder& vkc::ImageBuilder::SetFormat(VkFormat format)
{
	m_Format = format;
	return *this;
}

vkc::ImageBuilder& vkc::ImageBuilder::SetTiling(VkImageTiling tiling)
{
	m_Tiling = tiling;
	return *this;
}

vkc::ImageBuilder& vkc::ImageBuilder::SetAspectFlags(VkImageAspectFlags aspectFlags)
{
	m_AspectFlags = aspectFlags;
	return *this;
}

vkc::ImageBuilder& vkc::ImageBuilder::SetLayers(uint32_t layers)
{
	m_Layers = layers;
	return *this;
}

vkc::ImageBuilder& vkc::ImageBuilder::SetMipLevels(uint32_t mipLevels)
{
	m_MipLevels = mipLevels;
	return *this;
}

vkc::ImageBuilder& vkc::ImageBuilder::SetType(VkImageType type)
{
	m_ImageType = type;
	return *this;
}

vkc::ImageBuilder& vkc::ImageBuilder::SetExtent(VkExtent2D extent)
{
	m_Extent = extent;
	return *this;
}

vkc::ImageBuilder& vkc::ImageBuilder::SetMemoryUsage(VmaMemoryUsage memoryUsage)
{
	m_MemoryUsage = memoryUsage;
	return *this;
}

vkc::ImageBuilder& vkc::ImageBuilder::SetFlags(VkImageCreateFlags flags)
{
	m_CreationFlags = flags;
	return *this;
}

vkc::ImageBuilder& vkc::ImageBuilder::SetSharingMode(VkSharingMode sharingMode)
{
	m_SharingMode = sharingMode;
	return *this;
}

vkc::ImageBuilder& vkc::ImageBuilder::SetMemoryFlags(VmaAllocationCreateFlags createFlags)
{
	m_MemoryFlags = createFlags;
	return *this;
}

vkc::Image vkc::ImageBuilder::Build(VkImageUsageFlags usage, bool addToQueue) const
{
	VkImageCreateInfo createInfo{};
	createInfo.sType         = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	createInfo.pNext         = nullptr;
	createInfo.flags         = m_CreationFlags;
	createInfo.imageType     = m_ImageType;
	createInfo.format        = m_Format;
	createInfo.tiling        = m_Tiling;
	createInfo.extent.width  = m_Extent.width;
	createInfo.extent.height = m_Extent.height;
	createInfo.extent.depth  = 1;
	createInfo.arrayLayers   = m_Layers;
	createInfo.usage         = usage;
	createInfo.mipLevels     = 1;
	createInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	createInfo.sharingMode   = m_SharingMode;
	createInfo.samples       = VK_SAMPLE_COUNT_1_BIT;

	VmaAllocationCreateInfo vmaAllocationCreateInfo{};
	vmaAllocationCreateInfo.flags = m_MemoryFlags;
	vmaAllocationCreateInfo.usage = m_MemoryUsage;

	Image image{};
	image.m_AspectFlags = m_AspectFlags;
	image.m_Extent      = m_Extent;
	image.m_Format      = m_Format;
	image.m_Layers      = m_Layers;
	image.m_MipLevels   = m_MipLevels;
	image.m_Layouts.resize(m_Layers);
	for (auto& layers: image.m_Layouts)
		layers.resize(m_MipLevels, VK_IMAGE_LAYOUT_UNDEFINED);

	vmaCreateImage(m_Context.Allocator, &createInfo, &vmaAllocationCreateInfo, image, &image.m_Allocation, nullptr);

	if (addToQueue)
		m_Context.DeletionQueue.Push([context = &m_Context, image = image.m_Image, allocation = image.m_Allocation]
		{
			vmaDestroyImage(context->Allocator, image, allocation);
		});
	return image;
}
