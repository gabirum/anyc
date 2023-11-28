#include "ui.h"

#include <stdlib.h>
#include <memory.h>
#include <string.h>
#include <stdio.h>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <vulkan/vulkan.h>

#ifdef NDEBUG
#define ENABLE_VALIDATION_LAYERS false
#define DEBUG(format, ...)
#else
#define ENABLE_VALIDATION_LAYERS true
#define DEBUG(format, ...) fprintf(stderr, "%s:%d: " format "\n", __FILE__, __LINE__, ##__VA_ARGS__)
#endif

#define VALIDATION_LAYERS_SIZE 1

static char const *const VALIDATION_LAYERS[VALIDATION_LAYERS_SIZE] = {"VK_LAYER_KHRONOS_validation"};

GLFWwindow *init_window()
{
  glfwInit();

  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
  glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

  return glfwCreateWindow(1280, 720, "Vulkan", NULL, NULL);
}

bool check_validation_layer_support()
{
  uint32_t layer_count = 0;
  vkEnumerateInstanceLayerProperties(&layer_count, NULL);

  VkLayerProperties *available_layers = calloc(layer_count, sizeof(VkLayerProperties));
  vkEnumerateInstanceLayerProperties(&layer_count, available_layers);

  bool layer_found = false;
  for (size_t i = 0; i < VALIDATION_LAYERS_SIZE; i++)
  {
    char const *const layer_name = VALIDATION_LAYERS[i];

    for (uint32_t j = 0; j < layer_count; j++)
    {
      if (strcmp(layer_name, available_layers[j].layerName) == 0)
      {
        layer_found = true;
        break;
      }
    }
  }

  free(available_layers);
  return layer_found;
}

char const **get_required_extensions(uint32_t *count)
{
  *count = 0;
  char const *const *glfw_extensions = glfwGetRequiredInstanceExtensions(count);

  uint32_t new_count = ENABLE_VALIDATION_LAYERS ? *count + 1 : *count;

  char const **extensions = calloc(new_count, sizeof(char *));
  memcpy(extensions, glfw_extensions, *count * sizeof(char *));

  if (ENABLE_VALIDATION_LAYERS)
  {
    extensions[new_count - 1] = VK_EXT_DEBUG_UTILS_EXTENSION_NAME;
  }

  *count = new_count;
  return extensions;
}

VKAPI_ATTR VkBool32 VKAPI_CALL debug_callback(
    VkDebugUtilsMessageSeverityFlagBitsEXT message_severity,
    VkDebugUtilsMessageTypeFlagsEXT message_type,
    VkDebugUtilsMessengerCallbackDataEXT const *p_callback_data,
    void *p_user_data)
{
  DEBUG("validation layer: %s", p_callback_data->pMessage);

  return VK_FALSE;
}

void populate_debug_messenger_create_info(VkDebugUtilsMessengerCreateInfoEXT *p_create_info)
{
  *p_create_info = (VkDebugUtilsMessengerCreateInfoEXT){
      .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
      .messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,
      .messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
      .pfnUserCallback = debug_callback};
}

bool create_instance(VkInstance *p_instance)
{
  if (ENABLE_VALIDATION_LAYERS && !check_validation_layer_support())
    return false;

  VkApplicationInfo app_info = {
      .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
      .pApplicationName = "Vulkan",
      .applicationVersion = VK_MAKE_API_VERSION(0, 1, 0, 0),
      .pEngineName = "No Engine",
      .engineVersion = VK_MAKE_API_VERSION(0, 1, 0, 0),
      .apiVersion = VK_API_VERSION_1_0};

  uint32_t glfw_extension_count = 0;
  char const **glfw_extensions = get_required_extensions(&glfw_extension_count);

  VkInstanceCreateInfo create_info = {
      .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
      .pApplicationInfo = &app_info,
      .enabledExtensionCount = glfw_extension_count,
      .ppEnabledExtensionNames = glfw_extensions,
      .enabledLayerCount = 0};

  VkDebugUtilsMessengerCreateInfoEXT debug_create_info;

  if (ENABLE_VALIDATION_LAYERS)
  {
    create_info.enabledLayerCount = VALIDATION_LAYERS_SIZE;
    create_info.ppEnabledLayerNames = VALIDATION_LAYERS;

    populate_debug_messenger_create_info(&debug_create_info);
    create_info.pNext = &debug_create_info;
  }
  else
  {
    create_info.pNext = NULL;
  }

  bool result = vkCreateInstance(&create_info, NULL, p_instance) == VK_SUCCESS;
  free(glfw_extensions);

  return result;
}

VkResult create_debug_utils_messenger_EXT(
    VkInstance instance,
    VkDebugUtilsMessengerCreateInfoEXT const *p_create_info,
    VkAllocationCallbacks const *p_alloc,
    VkDebugUtilsMessengerEXT *p_debug_messenger)
{
  PFN_vkCreateDebugUtilsMessengerEXT func =
      (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");

  if (func != NULL)
    return func(instance, p_create_info, p_alloc, p_debug_messenger);
  else
    return VK_ERROR_EXTENSION_NOT_PRESENT;
}

bool setup_debug_messenger(VkInstance instance, VkDebugUtilsMessengerEXT *p_debug_messenger)
{
  if (!ENABLE_VALIDATION_LAYERS)
    return true;

  VkDebugUtilsMessengerCreateInfoEXT create_info;
  populate_debug_messenger_create_info(&create_info);

  return create_debug_utils_messenger_EXT(instance, &create_info, NULL, p_debug_messenger) == VK_SUCCESS;
}

bool is_device_suitable(
    VkPhysicalDevice device,
    VkSurfaceKHR surface,
    uint32_t *p_family_index,
    uint32_t *p_present_index)
{
  uint32_t queue_family_count = 0;
  vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count, NULL);

  VkQueueFamilyProperties *queue_families = calloc(queue_family_count, sizeof(VkQueueFamilyProperties));
  vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count, queue_families);

  uint32_t family_index = -1;
  uint32_t present_index = -1;
  for (uint32_t i = 0; i < queue_family_count; i++)
  {
    VkQueueFamilyProperties queue_family = queue_families[i];

    if (queue_family.queueFlags & VK_QUEUE_GRAPHICS_BIT)
      family_index = i;

    VkBool32 present_support = VK_FALSE;
    vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &present_support);

    if (present_support)
      present_index = i;
  }

  free(queue_families);

  if (family_index == -1 && present_index == -1)
    return false;

  *p_family_index = family_index;
  *p_present_index = present_index;

  return true;
}

bool pick_physical_device(
    VkInstance instance,
    VkPhysicalDevice *p_physical_device,
    VkSurfaceKHR surface,
    uint32_t *p_family_index,
    uint32_t *p_present_index)
{
  uint32_t device_count = 0;
  vkEnumeratePhysicalDevices(instance, &device_count, NULL);

  if (device_count == 0)
    return false;

  VkPhysicalDevice *devices = calloc(device_count, sizeof(VkPhysicalDevice));
  vkEnumeratePhysicalDevices(instance, &device_count, devices);

  bool has_device = false;
  for (uint32_t i = 0; i < device_count; i++)
  {
    VkPhysicalDevice device = devices[i];

    if (is_device_suitable(device, surface, p_family_index, p_present_index))
    {
      *p_physical_device = device;
      has_device = true;
      break;
    }
  }

  free(devices);

  return has_device;
}

bool create_logical_device(
    VkPhysicalDevice physical_device,
    uint32_t family_index,
    uint32_t present_index,
    VkDevice *p_device,
    VkQueue *p_graphics_queue,
    VkQueue *p_present_queue)
{
  uint32_t indices[2] = {family_index, present_index};
  VkDeviceQueueCreateInfo queue_create_infos[2] = {0};
  uint32_t indices_count = family_index == present_index ? 1 : 2;

  float queue_priority = 1.f;

  for (uint32_t i = 0; i < indices_count; i++)
  {
    VkDeviceQueueCreateInfo queue_create_info = {
        .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
        .queueFamilyIndex = indices[i],
        .queueCount = 1,
        .pQueuePriorities = &queue_priority};

    queue_create_infos[i] = queue_create_info;
  }

  VkPhysicalDeviceFeatures device_features = {0};
  VkDeviceCreateInfo create_info = {
      .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
      .pQueueCreateInfos = queue_create_infos,
      .queueCreateInfoCount = indices_count,
      .pEnabledFeatures = &device_features,
      .enabledExtensionCount = 0};

  if (ENABLE_VALIDATION_LAYERS)
  {
    create_info.enabledLayerCount = VALIDATION_LAYERS_SIZE;
    create_info.ppEnabledLayerNames = VALIDATION_LAYERS;
  }
  else
  {
    create_info.enabledLayerCount = 0;
  }

  if (vkCreateDevice(physical_device, &create_info, NULL, p_device) != VK_SUCCESS)
    return false;

  vkGetDeviceQueue(*p_device, family_index, 0, p_graphics_queue);
  vkGetDeviceQueue(*p_device, present_index, 0, p_present_queue);

  return true;
}

bool init_vulkan(
    VkInstance *p_instance,
    VkDebugUtilsMessengerEXT *p_debug_messenger,
    VkPhysicalDevice *p_physical_device,
    VkDevice *p_device,
    VkQueue *p_graphics_queue,
    VkQueue *p_present_queue,
    GLFWwindow *window,
    VkSurfaceKHR *p_surface)
{
  if (!create_instance(p_instance))
    return false;

  if (!setup_debug_messenger(*p_instance, p_debug_messenger))
    return false;

  if (glfwCreateWindowSurface(*p_instance, window, NULL, p_surface) != VK_SUCCESS)
    return false;

  uint32_t family_index = -1;
  uint32_t present_index = -1;
  if (!pick_physical_device(*p_instance, p_physical_device, *p_surface, &family_index, &present_index))
    return false;

  if (!create_logical_device(
          *p_physical_device,
          family_index,
          present_index,
          p_device,
          p_graphics_queue,
          p_present_queue))
    return false;

  return true;
}

void loop(GLFWwindow *window)
{
  while (!glfwWindowShouldClose(window))
  {
    glfwPollEvents();
  }
}

void destroy_debug_utils_messenger_EXT(
    VkInstance instance,
    VkDebugUtilsMessengerEXT debug_messenger,
    VkAllocationCallbacks const *p_alloc)
{
  PFN_vkDestroyDebugUtilsMessengerEXT func =
      (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
  if (func != NULL)
  {
    func(instance, debug_messenger, p_alloc);
  }
}

void cleanup(
    GLFWwindow *window,
    VkInstance instance,
    VkDebugUtilsMessengerEXT debug_messenger,
    VkDevice device,
    VkSurfaceKHR surface)
{
  vkDestroyDevice(device, NULL);

  if (ENABLE_VALIDATION_LAYERS)
  {
    destroy_debug_utils_messenger_EXT(instance, debug_messenger, NULL);
  }

  vkDestroySurfaceKHR(instance, surface, NULL);
  vkDestroyInstance(instance, NULL);

  glfwDestroyWindow(window);

  glfwTerminate();
}

bool init_app()
{
  GLFWwindow *window = init_window();

  VkInstance instance = NULL;
  VkDebugUtilsMessengerEXT debug = NULL;
  VkPhysicalDevice physical_device = VK_NULL_HANDLE;
  VkDevice device = NULL;
  VkQueue graphics_queue = NULL;
  VkQueue present_queue = NULL;
  VkSurfaceKHR surface = NULL;

  if (!init_vulkan(
          &instance,
          &debug,
          &physical_device,
          &device,
          &graphics_queue,
          &present_queue,
          window,
          &surface))
    return false;

  loop(window);

  cleanup(window, instance, debug, device, surface);

  return true;
}
