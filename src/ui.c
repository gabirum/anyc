#include "ui.h"

/* #define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <vulkan/vulkan.h>

#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#ifdef NDEBUG
#define ENABLE_VALIDATION_LAYERS false
#define DEBUG(format, ...)
#else
#define ENABLE_VALIDATION_LAYERS true
#define DEBUG(format, ...) fprintf(stderr, "%s:%d: " format "\n", __FILE__, __LINE__, ##__VA_ARGS__)
#endif

#define VALIDATION_LAYERS_SIZE 1

char const *validation_layers[VALIDATION_LAYERS_SIZE] = {"VK_LAYER_KHRONOS_validation"};

VkResult create_debug_utils_messenger_ext(
    VkInstance instance,
    VkDebugUtilsMessengerCreateInfoEXT const *create_info,
    VkAllocationCallbacks const *allocator,
    VkDebugUtilsMessengerEXT *debug_messenger)
{
  PFN_vkCreateDebugUtilsMessengerEXT func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");

  if (func == NULL)
    return VK_ERROR_EXTENSION_NOT_PRESENT;

  return func(instance, create_info, allocator, debug_messenger);
}

void destroy_debug_utils_messenger_ext(VkInstance instance, VkDebugUtilsMessengerEXT debug_messenger, VkAllocationCallbacks const *allocator)
{
  PFN_vkDestroyDebugUtilsMessengerEXT func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");

  if (func != NULL)
    func(instance, debug_messenger, allocator);
}

bool check_validation_layer_support()
{
  uint32_t count;
  vkEnumerateInstanceLayerProperties(&count, NULL);

  VkLayerProperties *available_layers = calloc(count, sizeof(VkLayerProperties));
  vkEnumerateInstanceLayerProperties(&count, available_layers);

  bool layer_found = false;

  for (size_t i = 0; i < VALIDATION_LAYERS_SIZE; i++)
  {
    char const *layer_name = validation_layers[i];

    for (uint32_t j = 0; j < count; j++)
    {
      if (strcmp(layer_name, available_layers[j].layerName) == 0)
      {
        layer_found = true;
        break;
      }
    }

    if (!layer_found)
      break;
  }

  free(available_layers);
  return layer_found;
}

void get_required_extensions()
{
  uint32_t extension_count = 0;
  char const **extensions = glfwGetRequiredInstanceExtensions(&extension_count);

  if (extensions == NULL)
  {
    DEBUG("no extensions found");
    return NULL;
  }

    alist_t *list = alist_create_fixed(extension_count + (ENABLE_VALIDATION_LAYERS ? 1 : 0), noop);

  for (uint32_t i = 0; i < extension_count; i++)
    alist_add(list, (void *)extensions[i]);

  if (ENABLE_VALIDATION_LAYERS)
    alist_add(list, VK_EXT_DEBUG_UTILS_EXTENSION_NAME);

  return list;
}

VKAPI_ATTR VkBool32 VKAPI_CALL debug_callback(
    VkDebugUtilsMessageSeverityFlagBitsEXT message_severity,
    VkDebugUtilsMessageTypeFlagsEXT message_type,
    VkDebugUtilsMessengerCallbackDataEXT const *callback_data,
    void *user_data)
{
  DEBUG("validation layer: %s", callback_data->pMessage);

  return VK_FALSE;
}

struct queue_families
{
  uint32_t graphics_family, present_family;
  bool has_graphics, has_present;
};

struct queue_families find_queue_families(VkPhysicalDevice device, VkSurfaceKHR surface)
{
  struct queue_families qf = {
      .graphics_family = 0,
      .present_family = 0,
      .has_graphics = false,
      .has_present = false};
  uint32_t queue_family_count = 0;
  vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count, NULL);

  VkQueueFamilyProperties *queue_families = calloc(queue_family_count, sizeof(VkQueueFamilyProperties));
  vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count, queue_families);

  for (uint32_t i = 0; i < queue_family_count; i++)
  {
    VkQueueFamilyProperties queue_family = queue_families[i];
    if (queue_family.queueFlags & VK_QUEUE_GRAPHICS_BIT)
    {
      qf.graphics_family = i;
      qf.has_graphics = true;
    }

    VkBool32 present_support = false;
    vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &present_support);

    if (present_support)
    {
      qf.present_family = i;
      qf.has_present = true;
    }

    if (qf.has_graphics && qf.has_present)
      break;
  }

  free(queue_families);
  return qf;
}

bool is_device_suitable(VkPhysicalDevice device, VkSurfaceKHR surface)
{
  struct queue_families qf = find_queue_families(device, surface);
  return qf.has_graphics && qf.has_present;
}

bool pick_physical_device(VkInstance instance, VkPhysicalDevice *_device, VkSurfaceKHR surface)
{
  uint32_t count = 0;
  vkEnumeratePhysicalDevices(instance, &count, NULL);

  if (count == 0)
  {
    DEBUG("No physical devices found");
    return false;
  }

  VkPhysicalDevice *devices = calloc(count, sizeof(VkPhysicalDevice));
  vkEnumeratePhysicalDevices(instance, &count, devices);

  for (uint32_t i = 0; i < count; i++)
  {
    VkPhysicalDevice device = devices[i];
    if (is_device_suitable(device, surface))
    {
      *_device = device;
      break;
    }
  }

  free(devices);

  if (*_device == VK_NULL_HANDLE)
  {
    DEBUG("No physical devices selected");
    return false;
  }

  return true;
}

bool comparer(void *a, void *b)
{
  return *(uint32_t *)a == *(uint32_t *)b;
}

uint64_t hasher(void *a)
{
  return *(uint32_t *)a;
}

struct each_family_arg
{
  float *queue_priority;
  alist_t *list;
};

void each_family(void *_family, void *_arg)
{
  uint32_t family = *(uint32_t *)_family;
  struct each_family_arg *arg = _arg;

  VkDeviceQueueCreateInfo *queue_info = calloc(1, sizeof(VkDeviceQueueCreateInfo));
  *queue_info = (VkDeviceQueueCreateInfo){
      .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
      .queueFamilyIndex = family,
      .queueCount = 1,
      .pQueuePriorities = arg->queue_priority,
  };
  alist_add(arg->list, queue_info);
}

bool create_logical_device(VkPhysicalDevice physical_device, VkDevice device, VkSurfaceKHR surface, VkQueue *graphics_queue, VkQueue *present_queue)
{
  bool result = true;
  struct queue_families qf = find_queue_families(physical_device, surface);

  alist_t *infos = alist_create(free);
  hset_t *unique_families = hset_create(.9, comparer, hasher, noop);
  hset_set(&unique_families, &qf.graphics_family);
  hset_set(&unique_families, &qf.present_family);

  float priority = 1.f;
  hset_for_each(unique_families, each_family, &(struct each_family_arg){.list = infos, .queue_priority = &priority});

  VkPhysicalDeviceFeatures features;
  VkDeviceCreateInfo create_info = {
      .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
      .queueCreateInfoCount = infos->capacity,
      .pQueueCreateInfos = (VkDeviceQueueCreateInfo const *)infos->data,
      .pEnabledFeatures = &features,
      .enabledExtensionCount = 0};

  if (ENABLE_VALIDATION_LAYERS)
  {
    create_info.enabledLayerCount = VALIDATION_LAYERS_SIZE;
    create_info.ppEnabledLayerNames = validation_layers;
  }
  else
  {
    create_info.enabledLayerCount = 0;
  }

  if (vkCreateDevice(physical_device, &create_info, NULL, &device) != VK_SUCCESS)
  {
    DEBUG("logical device not created");
    result = false;
    goto end;
  }

  vkGetDeviceQueue(device, qf.graphics_family, 0, graphics_queue);
  vkGetDeviceQueue(device, qf.present_family, 0, present_queue);

end:
  alist_dispose(&infos);
  hset_dispose(&unique_families);
  return result;
}

GLFWwindow *init_window()
{
  glfwInit();

  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
  glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
  return glfwCreateWindow(800, 600, "Vulkan", NULL, NULL);
}

void populate_debug_messenger_create_info(VkDebugUtilsMessengerCreateInfoEXT *info)
{
  VkDebugUtilsMessengerCreateInfoEXT create_info = {
      .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
      .messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,
      .messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
      .pfnUserCallback = debug_callback,
  };
  *info = create_info;
}

bool create_instance(VkInstance *instance)
{
  bool result = true;

  alist_t *extensions = NULL;

  if (ENABLE_VALIDATION_LAYERS && !check_validation_layer_support())
  {
    DEBUG("validation layers requested, but not available!");
    result = false;
    goto end;
  }

  VkApplicationInfo app_info = {
      .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
      .pApplicationName = "Triângulo",
      .applicationVersion = VK_MAKE_API_VERSION(0, 0, 1, 0),
      .pEngineName = "NO ENGINE",
      .engineVersion = VK_MAKE_API_VERSION(0, 0, 1, 0),
      .apiVersion = VK_API_VERSION_1_0};

  VkInstanceCreateInfo create_info = {
      .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
      .pApplicationInfo = &app_info};

  extensions = get_required_extensions();

  if (extensions == NULL)
  {
    result = false;
    goto end;
  }

  create_info.enabledExtensionCount = extensions->size;
  create_info.ppEnabledExtensionNames = (char const *const *)extensions->data;

  VkDebugUtilsMessengerCreateInfoEXT debug_create_info;
  if (ENABLE_VALIDATION_LAYERS)
  {
    create_info.enabledLayerCount = VALIDATION_LAYERS_SIZE;
    create_info.ppEnabledLayerNames = validation_layers;

    populate_debug_messenger_create_info(&debug_create_info);
    create_info.pNext = &debug_create_info;
  }
  else
  {
    create_info.enabledLayerCount = 0;
    create_info.pNext = NULL;
  }

  if (vkCreateInstance(&create_info, NULL, instance) != VK_SUCCESS)
  {
    DEBUG("instance creation error");
    result = false;
    goto end;
  }

end:
  if (extensions != NULL)
    alist_dispose(&extensions);

  return result;
}

bool setup_debugger(VkInstance instance, VkDebugUtilsMessengerEXT *debug_messenger)
{
  if (!ENABLE_VALIDATION_LAYERS)
    return true;

  VkDebugUtilsMessengerCreateInfoEXT create_info;
  populate_debug_messenger_create_info(&create_info);

  if (create_debug_utils_messenger_ext(instance, &create_info, NULL, debug_messenger) != VK_SUCCESS)
    return false;

  return true;
}

void app_loop(GLFWwindow *window)
{
  while (!glfwWindowShouldClose(window))
  {
    glfwPollEvents();
  }
}

void dispose(GLFWwindow *window, VkInstance instance, VkDebugUtilsMessengerEXT debugger, VkDevice device)
{
  vkDestroyDevice(device, NULL);

  if (ENABLE_VALIDATION_LAYERS)
    destroy_debug_utils_messenger_ext(instance, debugger, NULL);
  vkDestroyInstance(instance, NULL);

  glfwDestroyWindow(window);
  glfwTerminate();
} */

bool init_app()
{
  /* // init window
  GLFWwindow *window = init_window();

  // init vulkan
  VkInstance instance = NULL;
  if (!create_instance(&instance))
    return false;

  VkDebugUtilsMessengerEXT debugger = NULL;
  if (!setup_debugger(instance, &debugger))
    return false;

  VkSurfaceKHR surface = NULL;
  if (glfwCreateWindowSurface(instance, window, NULL, &surface) != VK_SUCCESS)
    return false;

  VkPhysicalDevice physical_device = NULL;
  if (!pick_physical_device(instance, &physical_device, surface))
    return false;

  VkDevice device = NULL;
  VkQueue graphics_queue = NULL;
  VkQueue present_queue = NULL;
  if (!create_logical_device(physical_device, device, surface, &graphics_queue, &present_queue))
    return false;

  app_loop(window);

  dispose(window, instance, debugger, device);
 */
  return true;
}
