# Mass API Plugin Documentation

## Table of Contents
1. [Overview](#overview)
2. [Installation & Setup](#installation--setup)
3. [Quick Start Guide](#quick-start-guide)
4. [Core Concepts](#core-concepts)
5. [C++ API Reference](#c-api-reference)
6. [Blueprint API Reference](#blueprint-api-reference)
7. [K2 Node System](#k2-node-system)
8. [Advanced Topics](#advanced-topics)
9. [Best Practices](#best-practices)
10. [Common Pitfalls](#common-pitfalls)
11. [Conclusion](#conclusion)

---

## Overview

The Mass API Plugin is a comprehensive wrapper for Unreal Engine's Mass Entity framework, designed to make the powerful but complex Mass Entity Component System (ECS) accessible to both C++ developers and Blueprint users. This plugin bridges the gap between the high-performance, data-oriented Mass Entity system and Unreal's traditional object-oriented workflow.

### Why Mass API?

The Mass Entity framework in Unreal Engine 5 provides exceptional performance for managing thousands or millions of entities through data-oriented design. However, its native API can be challenging to work with, especially in Blueprints. The Mass API plugin addresses these challenges by:

1. **Simplifying Entity Operations**: Providing intuitive functions for creating, modifying, and querying entities
2. **Blueprint Support**: Full Blueprint integration through custom K2 nodes and function libraries
3. **Type Safety**: Maintaining compile-time type checking while offering runtime flexibility
4. **Performance**: Zero-overhead abstractions that don't sacrifice the performance benefits of Mass Entity
5. **Dynamic Flags System**: A flexible 64-bit flag system for runtime entity state management without archetype changes

### When to Use Mass API vs Native Mass

**Use Mass API when:**
- Working outside of Mass processors (gameplay code, subsystems, actors)
- Spawning entities from Blueprint or gameplay events
- Performing simple entity queries and iterations
- Need Blueprint support for entity operations
- Want cleaner syntax for deferred operations

**Use Native Mass API when:**
- Inside Mass processors (for optimal performance)
- Performing chunk-based operations
- Need direct access to fragment views
- Working with complex archetype operations

### Key Features

- **Unified API**: Single subsystem (`UMassAPISubsystem`) as the primary entry point
- **Simplified Command Buffer Access**: New `Defer()` method for cleaner deferred operations
- **Entity Handles**: Blueprint-compatible wrapper (`FEntityHandle`) for Mass entity references
- **Query System**: Powerful entity filtering through `FEntityQuery` with All/Any/None logic and flag support
- **Template System**: Reusable entity templates for efficient batch creation
- **Flag Fragment**: Dynamic 64-bit flags that don't cause archetype migrations
- **Thread-Safe Deferred Operations**: Explicit command buffer parameters for safe multi-threaded execution
- **Full Fragment Support**: Works with all Mass fragment types (regular, shared, const shared, chunk, and tags)

---

## Installation & Setup

### Prerequisites

- Unreal Engine 5.6 or later
- MassGameplay plugin enabled
- C++ project (for full functionality)

### Installation Steps

1. **Add the Plugin**: Copy the MassAPI folder to your project's Plugins directory

2. **Enable Dependencies**: In your project's `.uproject` file or through the Plugins window, ensure these are enabled:
   - MassGameplay
   - MassEntity (automatically enabled with MassGameplay)
   - MassAPI

3. **Configure Build**: Add to your module's `.Build.cs` file:
   ```csharp
   PublicDependencyModuleNames.AddRange(new string[] { 
       "MassAPI",
       "MassEntity",
       "MassCommon",
       "StructUtils"
   });
   ```

4. **Include Headers**: In your C++ code:
   ```cpp
   #include "MassAPISubsystem.h"
   #include "MassAPIStructs.h"
   #include "MassAPIEnums.h"
   ```

---

## Quick Start Guide

### Creating Your First Entity (C++)

```cpp
// Step 1: Get the Mass API subsystem
UMassAPISubsystem& MassAPI = UMassAPISubsystem::GetRef(GetWorld());

// Step 2: Define a simple fragment
USTRUCT()
struct FHealthFragment : public FMassFragment
{
    GENERATED_BODY()
    
    UPROPERTY()
    float Health = 100.0f;
    
    UPROPERTY()
    float MaxHealth = 100.0f;
};

// Step 3: Define tags
USTRUCT()
struct FEnemyTag : public FMassTag
{
    GENERATED_BODY()
};

// Step 4: Create entities immediately
TArray<FMassEntityHandle> Enemies = MassAPI.BuildEntities(
    10,  // Quantity
    FEnemyTag{},
    FHealthFragment{100.0f, 100.0f},
    FEntityFlagFragment{}  // Always include for flag support
);

// Step 5: Use deferred operations with the new Defer() method
MassAPI.Defer()
    .AddTag<FProcessedTag>(Enemies[0])
    .RemoveFragment<FTempFragment>(Enemies[1])
    .DestroyEntity(Enemies[2]);
// Auto-flushes at end of frame
```

### Creating Entities in Blueprints

1. **Get the Mass API**: Use the "Get Mass API Subsystem" node
2. **Create Template**: Use "Create Entity Template" to define the entity composition
3. **Add Components**: Use "Set Fragment in Template" nodes to add data
4. **Build Entities**: Use "Build Entities from Template" to create multiple entities
5. **Modify Entities**: Use "Set Mass Fragment" nodes to change entity data

### Working Inside Mass Processors

```cpp
// Inside processors, prefer native Mass API for performance
UCLASS()
class UMyProcessor : public UMassProcessor
{
    GENERATED_BODY()
    
protected:
    FMassEntityQuery EntityQuery;  // Native Mass query
    
public:
    virtual void Execute(FMassEntityManager& EntityManager, 
                        FMassExecutionContext& Context) override
    {
        // Get MassAPI for specific operations only
        UMassAPISubsystem* MassAPI = UMassAPISubsystem::GetPtr(EntityManager.GetWorld());
        
        EntityQuery.ParallelForEachEntityChunk(EntityManager, Context,
            [&MassAPI, &Context](FMassExecutionContext& ChunkContext)
        {
            // BEST: Use Context.Defer() directly
            Context.Defer().AddTag<FProcessedTag>(Entity);
            
            // ALTERNATIVE (works but not recommended):
            // MassAPI->AddTag<FProcessedTag>(Context, Entity);
            // MassAPI->RemoveFragment<FOldFragment>(Context.Defer(), Entity);
            
            // Use MassAPI only for operations that require it (like spawning)
            if (MassAPI)
            {
                MassAPI->BuildEntityDefer(Context, FEffectTag{}, FTransformFragment{});
            }
        });
    }
};
```

---

## Core Concepts

### Command Buffer Access

The Defer() convenience method provides cleaner access to the global command buffer:

```cpp
// OLD way (verbose):
MassAPISubsystem->GetEntityManager()->Defer().AddTag<FDestroyTag>(Entity);

// NEW way (clean):
MassAPISubsystem->Defer().AddTag<FDestroyTag>(Entity);
```

**Important Context Rules:**
```cpp
// OUTSIDE processors - use MassAPI->Defer()
void GameplayFunction()
{
    UMassAPISubsystem& MassAPI = UMassAPISubsystem::GetRef(GetWorld());
    MassAPI.Defer().AddTag<FMyTag>(Entity);  // ✅ Correct
}

// INSIDE processors - best practice is Context.Defer()
void Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context)
{
    // ✅ BEST PRACTICE - Direct Context usage
    Context.Defer().AddTag<FMyTag>(Entity);
    
    // ⚠️ WORKS but NOT RECOMMENDED - MassAPI with Context parameter
    UMassAPISubsystem* MassAPI = UMassAPISubsystem::GetPtr(GetWorld());
    MassAPI->AddTag<FMyTag>(Context, Entity);           // Works but verbose
    MassAPI->AddTag<FMyTag>(Context.Defer(), Entity);   // Works but unnecessary
    
    // ❌ WRONG - Never use MassAPI.Defer() in processors
    MassAPI->Defer().AddTag<FMyTag>(Entity);  // Thread-unsafe!
}
```

**Why Context.Defer() is Best Practice:**
- More concise and readable
- Avoids unnecessary subsystem lookup
- Makes the processor context explicit
- Follows Unreal's Mass Entity conventions

### Entity Handle System

The plugin provides `FEntityHandle` as a Blueprint-compatible wrapper around `FMassEntityHandle`. This structure contains:

```cpp
USTRUCT(BlueprintType)
struct FEntityHandle
{
    UPROPERTY(BlueprintReadOnly)
    int32 Index;    // Entity index in the manager
    
    UPROPERTY(BlueprintReadOnly)  
    int32 Serial;   // Serial number for validity checking
};
```

Entity handles are lightweight references that can be safely passed around and stored. Always validate handles before use with `IsValid()`.

### Fragment Types

Mass API supports these Mass Entity fragment types:

1. **Regular Fragments** (`FMassFragment`): Per-entity data, most common type
2. **Tags** (`FMassTag`): Marker components with no data
3. **Shared Fragments** (`FMassSharedFragment`): Mutable data shared between entities
4. **Const Shared Fragments** (`FMassConstSharedFragment`): Immutable shared data
5. **Chunk Fragments** (`FMassChunkFragment`): Currently NOT supported

### Entity Flags System

The plugin introduces `FEntityFlagFragment`, a special fragment that stores 64 dynamic boolean flags per entity. This system allows runtime state changes without causing expensive archetype migrations:

```cpp
// Using flags in C++ (immediate operations only)
MassAPI.SetEntityFlag(Entity, EEntityFlags::Flag0);  // Set flag
bool HasFlag = MassAPI.HasEntityFlag(Entity, EEntityFlags::Flag0);  // Check flag
MassAPI.ClearEntityFlag(Entity, EEntityFlags::Flag0);  // Clear flag

// Flags in queries
FEntityQuery Query;
Query.AllFlags({EEntityFlags::Flag0, EEntityFlags::Flag1})  // Must have both
     .AnyFlags({EEntityFlags::Flag2, EEntityFlags::Flag3})  // Must have at least one
     .NoneFlags({EEntityFlags::Flag4});  // Must not have
```

### Query System

The `FEntityQuery` structure provides powerful entity filtering:

```cpp
FEntityQuery Query;

// Entities must have ALL of these
Query.All<FHealthFragment, FTransformFragment>();

// Entities must have ANY of these (at least one)
Query.Any<FPlayerTag, FAllyTag>();  

// Entities must have NONE of these
Query.None<FDeadTag, FInvisibleTag>();

// Can also use flags
Query.AllFlags({EEntityFlags::Flag0})
     .NoneFlags({EEntityFlags::Flag1});
```

### Performance Considerations

**Important:** The performance difference between `GetMatchingEntities` with a for loop and `ForEachEntityChunk` is not as significant as often assumed. For many use cases, especially with smaller entity counts or when you need random access to entities, `GetMatchingEntities` is perfectly acceptable:

```cpp
// This is NOT slow for most gameplay scenarios
TArray<FMassEntityHandle> Enemies = MassAPI.GetMatchingEntities(Query);
for (FMassEntityHandle Enemy : Enemies)
{
    // Process individual entities
    // Good for gameplay logic that needs entity handles
}
```

---

## C++ API Reference

### UMassAPISubsystem

The main entry point for all Mass API operations.

#### Getting the Subsystem

```cpp
// Get as pointer (can be null)
UMassAPISubsystem* MassAPI = UMassAPISubsystem::GetPtr(WorldContext);

// Get as reference (will assert if not available)
UMassAPISubsystem& MassAPI = UMassAPISubsystem::GetRef(WorldContext);
```

#### Command Buffer Access

```cpp
// Get the global command buffer
// WARNING: Only use outside of processor contexts!
FMassCommandBuffer& Defer() const;

// Usage examples:
MassAPI.Defer().AddTag<FMyTag>(Entity);
MassAPI.Defer().RemoveFragment<FOldFragment>(Entity);
MassAPI.Defer().DestroyEntity(Entity);
```

#### Entity Creation and Management

##### Immediate Entity Creation

```cpp
// Build a single entity with mixed fragments and tags
template<typename... TArgs>
FMassEntityHandle BuildEntity(TArgs&&... Args);

// Build multiple entities with mixed fragments and tags
template<typename... TArgs>
TArray<FMassEntityHandle> BuildEntities(int32 Quantity, TArgs&&... Args);

// Build entities from a template
TArray<FMassEntityHandle> BuildEntities(int32 Quantity, FMassEntityTemplateData& TemplateData);

// Reserve an entity handle without building
FMassEntityHandle ReserveEntity();

// Destroy entity immediately
void DestroyEntity(FMassEntityHandle EntityHandle);
void DestroyEntities(TArray<FMassEntityHandle>& EntityHandles);
```

##### Deferred Entity Creation

```cpp
// Defer entity creation using execution context (for processors)
// Returns reserved handle immediately, entity built when buffer flushes
template<typename... TArgs>
FMassEntityHandle BuildEntityDefer(FMassExecutionContext& Context, TArgs&&... Args);

// Defer entity creation using command buffer
template<typename... TArgs>
FMassEntityHandle BuildEntityDefer(FMassCommandBuffer& CommandBuffer, TArgs&&... Args);

// Defer entity creation from template
FMassEntityHandle BuildEntityDefer(FMassExecutionContext& Context, FMassEntityTemplateData& TemplateData);
FMassEntityHandle BuildEntityDefer(FMassCommandBuffer& CommandBuffer, FMassEntityTemplateData& TemplateData);

// Defer multiple entities from template
void BuildEntitiesDefer(FMassExecutionContext& Context, int32 Quantity, FMassEntityTemplateData& TemplateData, TArray<FMassEntityHandle>& OutEntityHandles);
void BuildEntitiesDefer(FMassCommandBuffer& CommandBuffer, int32 Quantity, FMassEntityTemplateData& TemplateData, TArray<FMassEntityHandle>& OutEntityHandles);

// Destroy entity deferred
void DestroyEntityDefer(FMassExecutionContext& Context, FMassEntityHandle EntityHandle);
void DestroyEntityDefer(FMassCommandBuffer& CommandBuffer, FMassEntityHandle EntityHandle);
```

#### Fragment Operations

```cpp
// Check if entity has fragment
template<typename T> 
bool HasFragment(FMassEntityHandle EntityHandle);

// Get fragment value (copies data)
template<typename T> 
T GetFragment(FMassEntityHandle EntityHandle);

// Get fragment pointer (for direct modification)
template<typename T> 
T* GetFragmentPtr(FMassEntityHandle EntityHandle);

// Set fragment value
template<typename T> 
void SetFragment(FMassEntityHandle EntityHandle, const T& Value);

// Add fragment to entity (immediate)
template<typename T> 
bool AddFragment(FMassEntityHandle EntityHandle);

// Remove fragment from entity (immediate)
template<typename T> 
bool RemoveFragment(FMassEntityHandle EntityHandle);

// Deferred fragment operations
template<typename T> 
void AddFragment(FMassExecutionContext& Context, FMassEntityHandle EntityHandle);

template<typename T> 
void AddFragment(FMassCommandBuffer& CommandBuffer, FMassEntityHandle EntityHandle);

template<typename T> 
void RemoveFragment(FMassExecutionContext& Context, FMassEntityHandle EntityHandle);

template<typename T> 
void RemoveFragment(FMassCommandBuffer& CommandBuffer, FMassEntityHandle EntityHandle);
```

#### Tag Operations

```cpp
// Check if entity has tag
template<typename T> 
bool HasTag(FMassEntityHandle EntityHandle);

// Add tag to entity (immediate)
template<typename T> 
bool AddTag(FMassEntityHandle EntityHandle);

// Remove tag from entity (immediate)
template<typename T> 
bool RemoveTag(FMassEntityHandle EntityHandle);

// Deferred tag operations
template<typename T> 
void AddTag(FMassExecutionContext& Context, FMassEntityHandle EntityHandle);

template<typename T> 
void AddTag(FMassCommandBuffer& CommandBuffer, FMassEntityHandle EntityHandle);

template<typename T> 
void RemoveTag(FMassExecutionContext& Context, FMassEntityHandle EntityHandle);

template<typename T> 
void RemoveTag(FMassCommandBuffer& CommandBuffer, FMassEntityHandle EntityHandle);
```

#### Shared Fragment Operations

```cpp
// Get shared fragment
template<typename T> 
FSharedStruct GetSharedFragment(FMassEntityHandle EntityHandle);

// Set shared fragment (creates shared instance)
template<typename T> 
FSharedStruct SetSharedFragment(FMassEntityHandle EntityHandle, const T& Value);

// Add shared fragment (deferred)
template<typename T> 
void AddSharedFragment(FMassExecutionContext& Context, FMassEntityHandle EntityHandle, const FSharedStruct& SharedFragment);

template<typename T> 
void AddSharedFragment(FMassCommandBuffer& CommandBuffer, FMassEntityHandle EntityHandle, const FSharedStruct& SharedFragment);

// Remove shared fragment (deferred)
template<typename T> 
void RemoveSharedFragment(FMassExecutionContext& Context, FMassEntityHandle EntityHandle);

template<typename T> 
void RemoveSharedFragment(FMassCommandBuffer& CommandBuffer, FMassEntityHandle EntityHandle);
```

#### Const Shared Fragment Operations

```cpp
// Get const shared fragment
template<typename T> 
FConstSharedStruct GetConstSharedFragment(FMassEntityHandle EntityHandle);

// Set const shared fragment (creates const shared instance)
template<typename T> 
FConstSharedStruct SetConstSharedFragment(FMassEntityHandle EntityHandle, const T& Value);

// Add const shared fragment (deferred)
template<typename T> 
void AddConstSharedFragment(FMassExecutionContext& Context, FMassEntityHandle EntityHandle, const FConstSharedStruct& ConstSharedFragment);

template<typename T> 
void AddConstSharedFragment(FMassCommandBuffer& CommandBuffer, FMassEntityHandle EntityHandle, const FConstSharedStruct& ConstSharedFragment);

// Remove const shared fragment (deferred)
template<typename T> 
void RemoveConstSharedFragment(FMassExecutionContext& Context, FMassEntityHandle EntityHandle);

template<typename T> 
void RemoveConstSharedFragment(FMassCommandBuffer& CommandBuffer, FMassEntityHandle EntityHandle);
```

#### Flag Operations (Immediate Only)

```cpp
// Set a flag on an entity
void SetEntityFlag(FMassEntityHandle EntityHandle, EEntityFlags Flag);

// Clear a flag on an entity
void ClearEntityFlag(FMassEntityHandle EntityHandle, EEntityFlags Flag);

// Toggle a flag on an entity
void ToggleEntityFlag(FMassEntityHandle EntityHandle, EEntityFlags Flag);

// Check if entity has a flag
bool HasEntityFlag(FMassEntityHandle EntityHandle, EEntityFlags Flag) const;

// Check if entity has all specified flags
bool HasAllEntityFlags(FMassEntityHandle EntityHandle, const TArray<EEntityFlags>& Flags) const;

// Check if entity has any of the specified flags
bool HasAnyEntityFlags(FMassEntityHandle EntityHandle, const TArray<EEntityFlags>& Flags) const;

// Check if entity has none of the specified flags
bool HasNoneEntityFlags(FMassEntityHandle EntityHandle, const TArray<EEntityFlags>& Flags) const;

// Get all flags as a bitmask
uint64 GetEntityFlags(FMassEntityHandle EntityHandle) const;

// Set all flags from a bitmask
void SetEntityFlags(FMassEntityHandle EntityHandle, uint64 FlagMask);
```

#### Query Operations

```cpp
// Get entities matching a query
TArray<FMassEntityHandle> GetMatchingEntities(const FEntityQuery& Query);

// Check if entity matches a query
bool MatchQuery(FMassEntityHandle EntityHandle, const FEntityQuery& Query);

// Get archetype composition for an entity
FMassArchetypeCompositionDescriptor GetEntityComposition(FMassEntityHandle EntityHandle);

// Get all entities (use with caution)
TArray<FMassEntityHandle> GetAllEntities();

// Count entities matching a query
int32 CountMatchingEntities(const FEntityQuery& Query);
```

#### Template Operations

```cpp
// Create a template from entities
FMassEntityTemplateData CreateTemplate(const TArray<FMassEntityHandle>& SampleEntities);

// Create an empty template
FMassEntityTemplateData CreateEmptyTemplate();

// Add fragment to template
template<typename T>
void AddFragmentToTemplate(FMassEntityTemplateData& Template, const T& Fragment);

// Add tag to template
template<typename T>
void AddTagToTemplate(FMassEntityTemplateData& Template);

// Build entities from template
TArray<FMassEntityHandle> BuildEntitiesFromTemplate(int32 Quantity, const FMassEntityTemplateData& Template);

// Build entities from template (deferred)
void BuildEntitiesFromTemplateDefer(FMassExecutionContext& Context, int32 Quantity, const FMassEntityTemplateData& Template, TArray<FMassEntityHandle>& OutHandles);

void BuildEntitiesFromTemplateDefer(FMassCommandBuffer& CommandBuffer, int32 Quantity, const FMassEntityTemplateData& Template, TArray<FMassEntityHandle>& OutHandles);
```

---

## Blueprint API Reference

### Getting Started with Blueprints

The Mass API provides comprehensive Blueprint support through:
- Custom K2 nodes for type-safe fragment operations
- Blueprint Function Library with utility functions
- Blueprint-compatible structs and enums

### Blueprint Function Library

#### System Functions

| Function | Description | Usage |
|----------|-------------|-------|
| **Get Mass API Subsystem** | Returns the Mass API subsystem instance | Call first to access all other functions |
| **Is Valid Entity** | Checks if an entity handle is valid | Always validate before operations |

#### Entity Creation Functions

| Function | Description | Parameters |
|----------|-------------|------------|
| **Build Entity** | Creates a single entity | Template data |
| **Build Entities** | Creates multiple entities | Quantity, Template |
| **Destroy Entity** | Destroys an entity immediately | Entity Handle |
| **Destroy Entities** | Destroys multiple entities | Array of Handles |

#### Fragment Operations

| Function | Description | Notes |
|----------|-------------|-------|
| **Has Mass Fragment** | Checks if entity has fragment | Returns boolean |
| **Get Mass Fragment** | Gets fragment value | Type-specific node |
| **Set Mass Fragment** | Sets fragment value | Type-specific node |
| **Add Mass Fragment** | Adds fragment to entity | Causes archetype change |
| **Remove Mass Fragment** | Removes fragment | Causes archetype change |

#### Tag Operations

| Function | Description | Notes |
|----------|-------------|-------|
| **Has Mass Tag** | Checks if entity has tag | Returns boolean |
| **Add Mass Tag** | Adds tag to entity | Causes archetype change |
| **Remove Mass Tag** | Removes tag | Causes archetype change |

#### Flag Operations

| Function | Description | Notes |
|----------|-------------|-------|
| **Set Entity Flag** | Sets a flag bit on entity | No archetype change |
| **Set Template Flag** | Sets a flag bit in template | For entity creation |
| **Clear Entity Flag** | Clears a flag bit | No archetype change |
| **Has Entity Flag** | Checks flag state | Returns boolean |

#### Query Operations

| Function | Description | Returns |
|----------|-------------|---------|
| **Get Matching Entities** | Finds entities matching query | Array of handles |
| **Match Query** | Checks if entity matches query | Boolean |

### Blueprint Workflow Examples

#### Example 1: Creating and Managing Enemies

1. **Create Enemy Template**:
   - Create FEntityTemplate variable
   - Add FEnemyTag to Tags array
   - Add FHealthFragment to Fragments array
   - Add FEntityFlagFragment to Fragments array

2. **Spawn Enemies**:
   - Get Mass API Subsystem
   - Call Build Entities with template
   - Store returned entity handles

3. **Update Enemies**:
   - For Each Loop over EntityHandles
   - Get Mass Fragment (FHealthFragment)
   - Modify health value
   - Set Mass Fragment with updated value

4. **Query Enemies**:
   - Create FEntityQuery variable
   - Set All list: [FHealthFragment, FEnemyTag]
   - Set None list: [FDeadTag]
   - Get Matching Entities with query
   - Process results

---

## K2 Node System

The plugin includes custom K2 nodes for advanced Blueprint integration. These nodes provide type-safe, user-friendly ways to work with Mass entities in Blueprint graphs.

### Custom K2 Nodes

The plugin implements specialized Blueprint nodes that extend the standard function call nodes:

#### Base Fragment Function Node (UK2Node_FragmentFunction)

All fragment-related K2 nodes inherit from this base class, which provides:
- Dynamic pin generation based on selected fragment type
- Type validation
- Automatic pin connection management

#### Fragment Access Nodes

| Node Class | Purpose | Special Features |
|------------|---------|------------------|
| **UK2Node_GetFragment** | Gets fragment from entity | Dynamically creates output pin matching fragment type |
| **UK2Node_SetFragment** | Sets fragment on entity | Dynamically creates input pin for fragment data |
| **UK2Node_GetSharedFragment** | Gets shared fragment | Handles shared fragment references |
| **UK2Node_SetSharedFragment** | Sets shared fragment | Manages shared instance creation |
| **UK2Node_GetConstSharedFragment** | Gets immutable shared fragment | Read-only access |
| **UK2Node_SetConstSharedFragment** | Sets immutable shared fragment | Creates const shared instances |

#### Template Nodes

| Node Class | Purpose | Special Features |
|------------|---------|------------------|
| **UK2Node_GetFragmentFromTemplate** | Extracts fragment from template | Template introspection |
| **UK2Node_SetFragmentInTemplate** | Adds fragment to template | Template building |
| **UK2Node_SetSharedFragmentInTemplate** | Adds shared fragment to template | Shared instance management |
| **UK2Node_SetConstSharedFragmentInTemplate** | Adds const shared fragment | Const instance management |

### Node Implementation Details

Each K2 node follows this pattern:

1. **Pin Allocation**: Creates pins based on the selected fragment type
2. **Type Validation**: Ensures only valid Mass fragment types are used
3. **Dynamic Updates**: Refreshes pins when fragment type changes
4. **Compilation**: Translates to efficient runtime calls

Example node structure:
```cpp
class UK2Node_GetFragment : public UK2Node_FragmentFunction
{
    // Creates pins: Exec, EntityHandle, FragmentType, OutFragment, Success
    virtual void AllocateDefaultPins() override;
    
    // Updates OutFragment pin when FragmentType changes
    virtual void OnFragmentTypeChanged() override;
    
    // Validates fragment type is derived from FMassFragment
    virtual bool IsValidFragmentStruct(const UScriptStruct* Struct) const override;
};
```

---

## Advanced Topics

### Performance Optimization

The Mass API plugin maintains the performance benefits of the Mass Entity system while adding convenience. Here are key optimization strategies:

#### Batch Operations
```cpp
// Good: Batch operations with command buffer
UMassAPISubsystem& MassAPI = UMassAPISubsystem::GetRef(GetWorld());
for (FMassEntityHandle Entity : Entities)
{
    // Chain operations using Defer()
    MassAPI.Defer()
        .AddTag<FProcessedTag>(Entity)
        .RemoveFragment<FTempDataFragment>(Entity);
}
// Flush automatically at end of frame

// Less optimal: Immediate operations
for (FMassEntityHandle Entity : Entities)
{
    MassAPI.AddTag<FProcessedTag>(Entity);  // Causes immediate archetype change
    MassAPI.RemoveFragment<FTempDataFragment>(Entity);  // Another archetype change
}
```

#### Query Caching
```cpp
// Cache queries that are used frequently
class AMyGameMode : public AGameMode
{
private:
    FEntityQuery CachedEnemyQuery;
    
public:
    virtual void BeginPlay() override
    {
        // Build query once
        CachedEnemyQuery.All<FEnemyDataFragment, FTransformFragment>()
                        .None<FDeadTag>();
    }
    
    void UpdateEnemies()
    {
        // Reuse cached query
        UMassAPISubsystem& MassAPI = UMassAPISubsystem::GetRef(GetWorld());
        TArray<FMassEntityHandle> Enemies = MassAPI.GetMatchingEntities(CachedEnemyQuery);
        
        for (FMassEntityHandle Enemy : Enemies)
        {
            // Process enemies
        }
    }
};
```

#### Flag System vs Tags

Use the flag system for frequently changing states to avoid archetype migrations:

```cpp
// Good: Using flags for dynamic states (no archetype change)
MassAPI.SetEntityFlag(Entity, EEntityFlags::Flag0);  // IsStunned
MassAPI.ClearEntityFlag(Entity, EEntityFlags::Flag0);

// Less optimal: Using tags for frequently changing states (causes archetype changes)
MassAPI.AddTag<FStunnedTag>(Entity);
MassAPI.RemoveTag<FStunnedTag>(Entity);
```

### Thread Safety

The Mass Entity system is designed for multi-threaded execution. When using Mass API:

1. **Entity Manager Access**: The entity manager is not thread-safe for modifications
2. **Read Operations**: Safe from multiple threads
3. **Write Operations**: Must be synchronized or use command buffers
4. **Deferred Operations**: Thread-safe when each thread uses its own command buffer

**Important Rule**: Inside processors, always prefer `Context.Defer()` for thread-safe deferred operations. While `MassAPI->AddTag(Context, Entity)` technically works, it's more verbose and less idiomatic. Never use `MassAPI.Defer()` inside processors as it's thread-unsafe.

### Memory Management

Understanding memory layout is crucial for performance:

```cpp
// Fragments are stored in chunks for cache efficiency
struct FMassArchetypeChunk
{
    // All entities with same archetype are grouped
    // Fragments are stored in Structure of Arrays (SoA) format
    // This ensures optimal cache usage during iteration
};

// Shared fragments use reference counting
FSharedStruct SharedData = MassAPI.SetSharedFragment(Entity, MySharedData);
// Multiple entities can reference the same shared instance
// Automatically cleaned up when last reference is removed
```

### Advanced Examples

#### Using the Defer() Method

```cpp
void ProcessEnemyWave()
{
    UMassAPISubsystem& MassAPI = UMassAPISubsystem::GetRef(GetWorld());
    
    // Query enemies
    FEntityQuery EnemyQuery;
    EnemyQuery.All<FEnemyTag>()
              .AllFlags({EEntityFlags::Flag0});  // Active flag
    
    TArray<FMassEntityHandle> Enemies = MassAPI.GetMatchingEntities(EnemyQuery);
    
    // Process with deferred operations (clean syntax)
    for (FMassEntityHandle Enemy : Enemies)
    {
        if (FHealthFragment* Health = MassAPI.GetFragmentPtr<FHealthFragment>(Enemy))
        {
            Health->Health -= 10.0f;
            
            if (Health->Health <= 0)
            {
                // Chain deferred operations using new Defer() method
                MassAPI.Defer()
                    .AddTag<FDeadTag>(Enemy)
                    .RemoveTag<FActiveTag>(Enemy)
                    .PushCommand([Enemy]() {
                        // Custom command
                        UE_LOG(LogTemp, Warning, TEXT("Enemy destroyed"));
                    });
            }
        }
    }
    // All deferred operations flush automatically at end of frame
}
```

#### Mixed Usage Pattern

```cpp
class AGameMode : public AGameModeBase
{
public:
    void SpawnAndProcessWave()
    {
        UMassAPISubsystem& MassAPI = UMassAPISubsystem::GetRef(GetWorld());
        
        // Immediate entity creation
        TArray<FMassEntityHandle> Wave = MassAPI.BuildEntities(
            20,
            FEnemyTag{},
            FHealthFragment{100.0f, 100.0f}
        );
        
        // Mix immediate and deferred operations
        for (int32 i = 0; i < Wave.Num(); ++i)
        {
            FMassEntityHandle Enemy = Wave[i];
            
            // Immediate operations for data modification
            if (FHealthFragment* Health = MassAPI.GetFragmentPtr<FHealthFragment>(Enemy))
            {
                Health->Health = 100.0f + (i * 10.0f);  // Varying health
            }
            
            // Immediate flag operations
            MassAPI.SetEntityFlag(Enemy, EEntityFlags::Flag0);  // Active
            
            // Deferred structural changes (clean syntax)
            if (i % 5 == 0)  // Every 5th enemy
            {
                MassAPI.Defer()
                    .AddTag<FEliteTag>(Enemy)
                    .AddFragment<FShieldFragment>(Enemy);
            }
        }
        
        // Deferred operations flush automatically
    }
};
```

---

## Best Practices

### Key Guidelines

1. **Context Rules**: 
   - Outside processors: Use `MassAPI.Defer()`
   - Inside processors: Use `Context.Defer()` (best practice)
   - Avoid: `MassAPI->AddTag(Context, Entity)` (works but verbose)
2. **Flag System**: Use flags for frequently changing states to avoid archetype migrations
3. **Query Caching**: Build queries once and reuse them
4. **Batch Operations**: Group archetype changes using deferred operations
5. **Entity Validation**: Always check entity handles before use
6. **Fragment Pointers**: Get pointers after archetype changes, not before

### Command Buffer Patterns

```cpp
// OUTSIDE PROCESSORS:
MassAPI.Defer().AddTag<FTag>(Entity);  // Use global buffer

// Or with custom buffer for explicit control:
FMassCommandBuffer CustomBuffer;
MassAPI.AddTag<FTag>(CustomBuffer, Entity);
CustomBuffer.Flush(*MassAPI.GetEntityManager());

// INSIDE PROCESSORS:
Context.Defer().AddTag<FTag>(Entity);  // BEST: Direct context usage

// Alternative approaches (work but not recommended):
MassAPI->AddTag<FTag>(Context, Entity);         // Verbose
MassAPI->AddTag<FTag>(Context.Defer(), Entity); // Unnecessary indirection
```

### Quick Decision Tree

```
Are you in a Mass Processor?
├── YES 
│   ├── Best Practice → Context.Defer().AddTag<>(Entity)
│   ├── Alternative → MassAPI->AddTag<>(Context, Entity) [works but verbose]
│   └── Never → MassAPI->Defer().AddTag<>(Entity) [thread-unsafe!]
└── NO  → Use MassAPI.Defer() for deferred operations
```

---

## Common Pitfalls

### 1. Using Wrong Command Buffer Context

**Problem**: Using `MassAPI.Defer()` inside processors
```cpp
// ❌ WRONG - Thread-unsafe in processors!
void Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context)
{
    MassAPI->Defer().AddTag<FTag>(Entity);
}
```

**Solution**: Use Context.Defer() in processors
```cpp
// ✅ BEST PRACTICE
void Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context)
{
    Context.Defer().AddTag<FTag>(Entity);
}

// ⚠️ WORKS but not recommended (verbose)
void Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context)
{
    UMassAPISubsystem* MassAPI = UMassAPISubsystem::GetPtr(GetWorld());
    MassAPI->AddTag<FTag>(Context, Entity);         // Works but unnecessary
    MassAPI->AddTag<FTag>(Context.Defer(), Entity); // Works but redundant
}
```

### 2. Fragment Pointer Invalidation

**Problem**: Storing fragment pointers across archetype changes
```cpp
// ❌ WRONG
FHealthFragment* Health = MassAPI.GetFragmentPtr<FHealthFragment>(Entity);
MassAPI.AddFragment<FShieldFragment>(Entity);  // Archetype change!
Health->Value = 100.0f;  // Pointer is now invalid!
```

**Solution**: Get pointers after archetype changes
```cpp
// ✅ CORRECT
MassAPI.AddFragment<FShieldFragment>(Entity);
FHealthFragment* Health = MassAPI.GetFragmentPtr<FHealthFragment>(Entity);
Health->Value = 100.0f;
```

### 3. Forgetting Entity Flag Fragment

**Problem**: Trying to use flags without the flag fragment
```cpp
// ❌ WRONG
FMassEntityHandle Entity = MassAPI.BuildEntity(FEnemyTag{});
MassAPI.SetEntityFlag(Entity, EEntityFlags::Flag0);  // Crash or undefined behavior!
```

**Solution**: Always include FEntityFlagFragment
```cpp
// ✅ CORRECT
FMassEntityHandle Entity = MassAPI.BuildEntity(
    FEnemyTag{},
    FEntityFlagFragment{}  // Required for flag operations
);
MassAPI.SetEntityFlag(Entity, EEntityFlags::Flag0);
```

### 4. Query System Misunderstanding

**Problem**: Expecting FEntityQuery to have iteration methods
```cpp
// ❌ WRONG - FEntityQuery doesn't have ForEachEntityChunk
FEntityQuery Query;
Query.ForEachEntityChunk(...);  // This doesn't exist!
```

**Solution**: Use GetMatchingEntities for query results
```cpp
// ✅ CORRECT
FEntityQuery Query;
TArray<FMassEntityHandle> Results = MassAPI.GetMatchingEntities(Query);
for (FMassEntityHandle Entity : Results) { /* ... */ }
```

### 5. Performance Anti-Patterns

**Problem**: Creating queries every frame
```cpp
// ❌ WRONG - Inefficient
void Tick()
{
    FEntityQuery Query;
    Query.All<FHealthFragment>();  // Building query every tick
    auto Results = MassAPI.GetMatchingEntities(Query);
}
```

**Solution**: Cache and reuse queries
```cpp
// ✅ CORRECT - Efficient
FEntityQuery CachedQuery;

void BeginPlay()
{
    CachedQuery.All<FHealthFragment>();  // Build once
}

void Tick()
{
    auto Results = MassAPI.GetMatchingEntities(CachedQuery);  // Reuse
}
```

---

---

## Conclusion

The Mass API Plugin provides a powerful convenience layer for Unreal Engine's Mass Entity system. The Defer() method makes deferred operations intuitive, while maintaining full compatibility with the native Mass Entity API.

Remember the golden rules:
- Use `MassAPI.Defer()` outside processors
- Use `Context.Defer()` inside processors (best practice)
  - While `MassAPI->AddTag(Context, Entity)` works, it's more verbose
- Include `FEntityFlagFragment` for flag support
- Cache queries for better performance
- Validate entity handles before use

This documentation represents the complete state of the Mass API Plugin, providing a robust foundation for building high-performance entity systems while maintaining the ease of use expected in modern Unreal Engine development.
