#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#  define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#  define NOMINMAX
#endif

#if defined(_WIN32)
#  include <Windows.h>
#endif

#include <wrl.h>

#if defined(ENABLE_D3D12)

#include <d3d12.h>
#include <dxgi1_6.h>
#include <d3dcompiler.h>
#include <DirectXMath.h>
#include <render/backend/dx12/d3dx12.h>

#  if defined(USE_DXC)
#    include <dxcapi.h>
#  endif
#endif


#include <string>
#include <vector>
#include <functional>
#include <memory>
#include <mutex>
#include <cstdio>
#include <sstream>
#include <fstream>
#include <stdexcept>
#include <cassert>
#include <filesystem>
#include <functional>
#include <cassert>
#include <cstdarg>
#include <cstring>
#include <iterator>
#include <algorithm>
#include <cstdint>
#include <type_traits>
#include <optional>
#include <utility>
#include <iostream>
#include <unordered_map>
#include <unordered_set>
#include <array>
#include <variant>
#include <atomic>
#include <chrono>
#include <thread>
#include <condition_variable>
#include <locale>
#include <iomanip>

