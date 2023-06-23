#if !defined(PLATFORM_H)

#define _CRT_SECURE_NO_WARNINGS

#include <stdint.h>
#include <cstring>
#include <intrin.h>

#include "types.h"


//TODO: This should be universal. Currently only works on windows. (make #define for each platform for this macro?)
//    : Also does nothing when compiling game as dll separate since game doesn't include windows outputdebug.
#if PLATFORM_COMPILED
#define MY_PRINTF(...) {char cad[512]; sprintf_s(cad, __VA_ARGS__);  OutputDebugString(cad);}
#else 
#define MY_PRINTF(...)
#endif


#define InvalidCodePath *((int*)0) = 0
#define InvalidDefaultCase default: {InvalidCodePath;} break

#if DEBUG
#define Assert(Expression) if ((Expression) != true) InvalidCodePath
#else
#define Assert(Expression)
#endif

#define ArrayCount(Array) sizeof(Array) / sizeof(Array[0])

#define AppendLine__(name,line) name##_##line
#define AppendLine_(name,line) AppendLine__(name,line)
#define AppendLine(name) AppendLine_(name, __LINE__)


#define Kilobytes(Value) ((Value)*1024LL)
#define Megabytes(Value) (Kilobytes(Value)*1024LL)
#define Gigabytes(Value) (Megabytes(Value)*1024LL)
#define Terabytes(Value) (Gigabytes(Value)*1024LL)


global bool global_logging_enabled = true;
#define LoggingStop global_logging_enabled = false
#define LoggingStart global_logging_enabled = true


#define foreach(thing, arr) for(u32 i = 0; i<ArrayCount(arr); i++) { thing = arr + i;
#define foreachcount(thing, arr, count) for(u32 i=0; i<count; i++) { thing = arr + i;
#define forend }

#define Case_Return(c, r) case c: return r

#define FloatEpsilon(var, val) if (var >= val - EPSILON && var <= val + EPSILON) var = val;

#define SetBit(bitfield, bit)   bitfield = bitfield |  (1<<bit)
#define UnsetBit(bitfield, bit) bitfield = bitfield & ~(1<<bit)


typedef struct thread_context {
	int placeholder; //we will use this in the future.
} thread_context;




#if DEBUG

/*
Will need to update in the future to read files faster/better/streaming?
*/
typedef struct debug_read_file_result {
	u32 contentSize;
	void* contents;
} debug_read_file_result;

#define DEBUG_PLATFORM_READ_ENTIRE_FILE(name) debug_read_file_result name(thread_context *thread, char *filename)
typedef DEBUG_PLATFORM_READ_ENTIRE_FILE(debug_platform_read_entire_file);

#define DEBUG_PLATFORM_FREE_FILE_FROM_MEMORY(name) void name(thread_context *thread, void *fileContents)
typedef DEBUG_PLATFORM_FREE_FILE_FROM_MEMORY(debug_platform_free_file_from_memory);

#define DEBUG_PLATFORM_WRITE_FILE(name) bool name(thread_context *thread, void* data, u32 size, char* filename)
typedef DEBUG_PLATFORM_WRITE_FILE(debug_platform_write_file);


#if _MSC_VER
#define BEGIN_BLOCK(COUNTER) COUNTER = __rdtsc();
#define END_BLOCK_(COUNTER) COUNTER = __rdtsc() - COUNTER;
#define END_BLOCK(INDEX, COUNTER) u64 EndCycleCount = __rdtsc() - COUNTER; debug_records[INDEX].cycle_count += EndCycleCount;
#else 
#define BEGIN_BLOCK(...)
#define END_BLOCK(...)
#endif


global s64 global_stack_bottom;
#define STACK_BOTTOM u8 local_stack_base_variable = 0; global_stack_bottom = (s64)(&local_stack_base_variable);

#else

#define STACK_BOTTOM

#endif 


enum Shader_Input_Type {
	ShaderInputType_Float1,
	ShaderInputType_Float2,
	ShaderInputType_Float3,
	ShaderInputType_Float4,
};


typedef struct Shader_Input {
	char name[20];
	Shader_Input_Type type;
    u16 semantic_index;
} Shader_Input;
typedef struct Shader_Input_Layout {
    u8 buffer_index_vertex;
	Shader_Input inputs_per_vertex[16];
	u32 inputs_per_vertex_count;
    u8 buffer_index_instance;
    Shader_Input inputs_per_instance[16];
    u32 inputs_per_instance_count;
} Shader_Input_Layout;



typedef struct Vertex {
    v3 point;
    v4 color;
    v2 uv;
    v3 normal;
} Vertex;

typedef struct Vertex_Base {
    v3 point;
    v2 uv;
    v3 normal;
} Vertex_Base;

typedef struct Vertex_Instance {
    v3 point;
    v4 color;
} Vertex_Instance;

typedef struct Vertex_Flat {
    v3 point;
    v4 color;
} Vertex_Flat;


//https://learn.microsoft.com/en-us/windows/win32/direct3dhlsl/dx-graphics-hlsl-packing-rules?redirectedfrom=MSDN
typedef struct Shader_Constants {
    f32 window_size_x;
    f32 window_size_y;
    f32 delta_time;
    f32 time;
    m44 m_u2p;
    m44 m_cam;
    m44 m_w2c;
    m44 m_c2p;
    m44 m_w2p;
    f32 view_pos[3];
    f32 extra_bytes;
} Shader_Constants;

typedef struct Shader_Object_Constants {
    v3 position;
    v4 color;
} Shader_Object_Constants;



typedef struct audio_buffer {
	u32 samplesPerSecond;
	u16 bytesPerSample; //NOTE : each sample is interleaved 16 bit left channel 16 bit right channel.
	u16 channels;
	u16 frameBytes;
	u32 bytesToLoadThisFrame;
	u32 byteCount;
	u32 bufferSize;
	void* data; 
} audio_buffer;




enum Render_Pass {
    RenderPass_World,
    RenderPass_UI,
    RenderPass_Count
};

enum Vertex_Type {
    VertexType_Standard_Instanced,
    VertexType_Standard,
    VertexType_Flat,
};

enum Render_Type {
    RenderType_TriangleList,
    RenderType_LineList,
};


typedef struct Render_Object {
	u16 shader_index;
	s16 texture_index = -1;
    
    Render_Type render_type;
	Vertex_Type vert_type;
    u32 vert_offset;
    u32 vert_count;
} Render_Object;







#define MAX_RENDER_OBJECTS 100000


#define MAX_VERTS 200000 //max verts to HARD render non-instanced
#define MAX_VERTS_BASE 3072 //max storage for instancable vertices
#define MAX_VERTS_INSTANCE 100000 //max instances to render at any given time
#define MAX_VERTS_IDX 60000 //max number of vertices that can be indexed
#define MAX_VERTS_FLAT 200000


typedef struct Render_Buffer {
    Shader_Constants shader_constants;
    
	Render_Object* objects[RenderPass_Count];
    Vertex_Base* verts_base;    // PRELOADED
    Vertex_Instance* verts_instance;
    u16* verts_idx;
	Vertex* verts;
    Vertex_Flat* verts_flat;
    
    u32 objectCounts[RenderPass_Count];
    u32 vertOffsets[RenderPass_Count];
    
    
    u32 verts_instance_count;
    u32 verts_idx_count;
	u32 verts_count;
    u32 verts_flat_count;
    u16 verts_base_count;
	
    u16 width;
	u16 height;
    
} Render_Buffer;






#define PLATFORM_LOAD_SHADER(name) u16 name(thread_context *thread, char* shaderFileName, Shader_Input_Layout *inputs)
typedef PLATFORM_LOAD_SHADER(platform_load_shader);
#define PLATFORM_UNLOAD_SHADER(name) void name(thread_context *thread, s16 shader_index)
typedef PLATFORM_UNLOAD_SHADER(platform_unload_shader);
#define PLATFORM_LOAD_TEXTURE(name) u16 name(thread_context *thread, u16 width, u16 height, u8* data)
typedef PLATFORM_LOAD_TEXTURE(platform_load_texture);
#define PLATFORM_UNLOAD_TEXTURE(name) void name(thread_context *thread, s16 texture_index)
typedef PLATFORM_UNLOAD_TEXTURE(platform_unload_texture);

#define PLATFORM_INIT_VERTEX_BASE(name) void name(thread_context *thread, Render_Buffer* buffer)
typedef PLATFORM_INIT_VERTEX_BASE(platform_init_vertex_base);



typedef struct Platform_Work_Queue platform_work_queue;

#define PLATFORM_WORK_QUEUE_CALLBACK(name) void name(platform_work_queue* queue, void* data)
typedef PLATFORM_WORK_QUEUE_CALLBACK(platform_work_queue_callback);

typedef struct Platform_Work_Queue_Entry {
	bool valid;
	platform_work_queue_callback* callback;
	void* data;
} Platform_Work_Queue_Entry;


#define PLATFORM_ADD_ENTRY(name) void name(Platform_Work_Queue* queue, platform_work_queue_callback* callback, void* data)
typedef PLATFORM_ADD_ENTRY(platform_add_entry);
#define PLATFORM_COMPLETE_ALL_WORK(name) void name(Platform_Work_Queue* queue)
typedef PLATFORM_COMPLETE_ALL_WORK(platform_complete_all_work);

#define PLATFORM_LOG(name) void name(char* format, ...)
typedef PLATFORM_LOG(platform_log);

#define PLATFORM_FORMAT_STRING(name) void name(char* buffer, char* format, ...)
typedef PLATFORM_FORMAT_STRING(platform_format_string);

#define PLATFORM_RUN(name) void name(char* dir, char* command_line);
typedef PLATFORM_RUN(platform_run);


typedef struct platform_api {
    bool initialized;
    
	platform_load_shader* platformLoadShader;
	platform_unload_shader* platformUnloadShader;
	platform_load_texture* platformLoadTexture;
	platform_unload_texture* platformUnloadTexture;
    
    platform_init_vertex_base* platformInitVertexBase;
    
	debug_platform_read_entire_file* DEBUGPlatformReadEntireFile;
	debug_platform_free_file_from_memory* DEBUGPlatformFreeFileFromMemory;
	debug_platform_write_file* DEBUGPlatformWriteFile;
    
	Platform_Work_Queue* highPriorityQueue;
	Platform_Work_Queue* lowPriorityQueue;
	platform_add_entry* platformAddEntry;
	platform_complete_all_work* platformCompleteAllWork;
	platform_log* log;
    platform_format_string* formatString;
    platform_run* platformRun;
    
} platform_api;

typedef struct game_memory {
    
    u64 primary_storage_size;
    void* primary_storage;
    
    u64 secondary_storage_size;
    void* secondary_storage;
    
    u64 debug_storage_size;
    void* debug_storage;
    
    u64 editor_storage_size;
    void* editor_storage;
    
    u64 stack_storage_size;
    void* stack_storage;
    
	platform_api platform_api;
    
} game_memory;










typedef struct Game_Controller_Button {
	bool down, hold, up;
	float actionTime;
}Game_Controller_Button;

typedef struct Game_Controller {
	bool isConnected;
	float stickX;
	float stickY;
    
    Game_Controller_Button any_button;
    
	union {
		Game_Controller_Button buttons[39];
		struct {
			Game_Controller_Button moveUp;
			Game_Controller_Button moveDown;
			Game_Controller_Button moveLeft;
			Game_Controller_Button moveRight;
            
            
			Game_Controller_Button actionUp;
			Game_Controller_Button actionDown;
			Game_Controller_Button actionLeft;
			Game_Controller_Button actionRight;
            
            Game_Controller_Button leftShoulder;
			Game_Controller_Button rightShoulder;
            
			Game_Controller_Button select;
            Game_Controller_Button start;
            
			Game_Controller_Button d0;
			Game_Controller_Button d1;
			Game_Controller_Button d2;
			Game_Controller_Button d3;
            Game_Controller_Button d4;
			Game_Controller_Button d5;
			Game_Controller_Button d6;
			Game_Controller_Button d7;
			Game_Controller_Button d8;
			Game_Controller_Button d9;
            
            Game_Controller_Button dReturn;
			Game_Controller_Button dPageUp;
			Game_Controller_Button dPageDown;
			Game_Controller_Button dHome;
			Game_Controller_Button dEnd;
			Game_Controller_Button dInsert;
			Game_Controller_Button dDelete;
            Game_Controller_Button dBackspace;
			Game_Controller_Button dBacktick;
            Game_Controller_Button dTab;
            Game_Controller_Button dCtrl;
            Game_Controller_Button dAlt;
            Game_Controller_Button dEsc;
            Game_Controller_Button dd;
            Game_Controller_Button dr;
            Game_Controller_Button ds;
            Game_Controller_Button dz;
            
            
			//to test when iterating.
			Game_Controller_Button terminator_fake;
		};
	};
}Game_Controller;


bool control_key_down(Game_Controller *controller, Game_Controller_Button key) {
    if (controller->dCtrl.hold) {
        if (key.down) {
            return true;
        }
    }
    return false;
}

bool control_key_hold(Game_Controller *controller, Game_Controller_Button key) {
    if (controller->dCtrl.hold) {
        if (key.hold) {
            return true;
        }
    }
    return false;
}

bool alt_key_down(Game_Controller *controller, Game_Controller_Button key) {
    if (controller->dAlt.hold) {
        if (key.down) {
            return true;
        }
    }
    return false;
}

bool alt_key_hold(Game_Controller *controller, Game_Controller_Button key) {
    if (controller->dAlt.hold) {
        if (key.hold) {
            return true;
        }
    }
    return false;
}


typedef struct Game_Mouse {
	u16 x;
	u16 y;
    s16 delta_x;
    s16 delta_y;
    s16 scroll_delta;
    bool locked;
    
	union {
		Game_Controller_Button buttons[4];
		struct {
			Game_Controller_Button left;
			Game_Controller_Button right;
			Game_Controller_Button middle;
			Game_Controller_Button terminator_fake;
		};
	};
}Game_Mouse;


//NOTE: all of these input codes are stolen from actual codes I don't care about
//except for the codes flagged as REAL

#define Input_Left_Mouse  1  //REAL
#define Input_Right_Mouse 2  //REAL
#define Input_Arrow_Up    3
#define Input_Arrow_Down  4
#define Input_Arrow_Left  5
#define Input_Arrow_Right 6
#define Input_Backspace   8  //REAL
#define Input_Page_Up     9
#define Input_Page_Down   10
#define Input_Home        11
#define Input_End         12
#define Input_Enter       13 //REAL
#define Input_Tab         14
#define Input_Esc         15
#define Input_Delete      127//REAL

typedef struct Game_Input {
	float delta_time;
    float frame_millis;
	Game_Mouse mouse;
	Game_Controller controllers[4];
    bool quit = false;
    
    bool char_typed;
    char last_char_typed;      //NOTE: should be from keydown
    char input_queue[128];
    u8   input_queue_count;
    bool key_repeat;           //windows sends multiple keydown messages after 1 second of holding.
} Game_Input;


typedef struct Game_Output {
    bool mouse_locked;
    bool mouse_was_locked;
    u16 mouse_prelock_x;
    u16 mouse_prelock_y;
} Game_Output;


s16 swallow_s16(s16* p) {
    s16 result = *p;
    *p = 0;
    return result;
}

#define Swallow_Value(src, dest) dest = *src; *src = 0

#define GAME_UPDATE_AND_RENDER(name) bool name(thread_context* thread, game_memory* memory, Render_Buffer* buffer, Game_Input input, Game_Output *output)
typedef GAME_UPDATE_AND_RENDER(game_update_and_render);

#define GAME_GET_AUDIO(name) void name(thread_context* thread, game_memory* memory, audio_buffer* buffer, float deltaTime)
typedef GAME_GET_AUDIO(game_get_audio);


#define GAME_LOAD_ASSETS(name) void name(thread_context* thread, game_memory* memory, Render_Buffer* buffer)
typedef GAME_LOAD_ASSETS(game_load_assets);


#define PLATFORM_H
#endif