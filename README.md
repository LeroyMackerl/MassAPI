````markdown
# Mass API Plugin Documentation

## Table of Contents
* [Overview](#overview)
* [Installation & Setup](#installation--setup)
* [Quick Start Guide](#quick-start-guide)
* [Core Concepts](#core-concepts)
* [C++ API Reference](#c-api-reference)
* [Blueprint API Reference](#blueprint-api-reference)
* [K2 Node System](#k2-node-system)
* [Advanced Topics](#advanced-topics)
* [Best Practices](#best-practices)
* [Common Pitfalls](#common-pitfalls)
* [Conclusion](#conclusion)

---

## Overview
The Mass API Plugin is a comprehensive wrapper for Unreal Engine's Mass Entity framework, designed to make the powerful but complex Mass Entity Component System (ECS) accessible to both C++ developers and Blueprint users. This plugin bridges the gap between the high-performance, data-oriented Mass Entity system and Unreal's traditional object-oriented workflow.

### Why Mass API?
The Mass Entity framework in Unreal Engine 5 provides exceptional performance for managing thousands or millions of entities through data-oriented design. However, its native API can be challenging to work with, especially in Blueprints. The Mass API plugin addresses these challenges by:

* **Simplifying Entity Operations**: Providing intuitive functions for creating, modifying, and querying entities
* **Blueprint Support**: Full Blueprint integration through custom K2 nodes and function libraries
* **Type Safety**: Maintaining compile-time type checking while offering runtime flexibility
* **Performance**: Zero-overhead abstractions that don't sacrifice the performance benefits of Mass Entity
* **Dynamic Flags System**: A flexible 64-bit flag system for runtime entity state management without archetype changes

### When to Use Mass API vs Native Mass

**Use Mass API when:**
* Working outside of Mass processors (gameplay code, subsystems, actors)
* Spawning entities from Blueprint or gameplay events
* Performing simple entity queries and iterations
* Need Blueprint support for entity operations
* Want cleaner syntax for deferred operations

**Use Native Mass API when:**
* Inside Mass processors (for optimal performance)
* Performing chunk-based operations
* Need direct access to fragment views
* Working with complex archetype operations

### Key Features
* **Unified API**: Single subsystem (`UMassAPISubsystem`) as the primary entry point
* **Simplified Command Buffer Access**: New `Defer()` method for cleaner deferred operations
* **Entity Handles**: Blueprint-compatible wrapper (`FEntityHandle`) for Mass entity references
* **Query System**: Powerful entity filtering through `FEntityQuery` with All/Any/None logic and flag support
* **Template System**: Reusable entity templates for efficient batch creation
* **Flag Fragment**: Dynamic 64-bit flags that don't cause archetype migrations
* **Thread-Safe Deferred Operations**: Explicit command buffer parameters for safe multi-threaded execution
* **Full Fragment Support**: Works with all Mass fragment types (regular, shared, const shared, chunk, and tags)

---

## Installation & Setup

### Prerequisites
* Unreal Engine 5.6 or later
* `MassGameplay` plugin enabled
* C++ project (for full functionality)

### Installation Steps
1.  **Add the Plugin**: Copy the `MassAPI` folder to your project's `Plugins` directory
2.  **Enable Dependencies**: In your project's `.uproject` file or through the Plugins window, ensure these are enabled:
    * `MassGameplay`
    * `MassEntity` (automatically enabled with `MassGameplay`)
    * `MassAPI`
3.  **Configure Build**: Add to your module's `.Build.cs` file:
    ```cpp
    PublicDependencyModuleNames.AddRange(new string[] { 
        "MassAPI",
        "MassEntity",
        "MassCommon",
        "StructUtils"
    });
    ```
4.  **Include Headers**: In your C++ code:
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
````

### Creating Entities in Blueprints

1.  **Get the Mass API**: Use the "Get Mass API Subsystem" node
2.  **Create Template**: Use "Create Entity Template" to define the entity composition
3.  **Add Components**: Use "Set Fragment in Template" nodes to add data
4.  **Build Entities**: Use "Build Entities from Template" to create multiple entities
5.  **Modify Entities**: Use "Set Mass Fragment" nodes to change entity data

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

-----

## Core Concepts

### Command Buffer Access

The `Defer()` convenience method provides cleaner access to the global command buffer:

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

**Why `Context.Defer()` is Best Practice:**

  * More concise and readable
  * Avoids unnecessary subsystem lookup
  * Makes the processor context explicit
  * Follows Unreal's Mass Entity conventions

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

Mass API supports all Mass Entity fragment types:

  * **Regular Fragments** (`FMassFragment`): Per-entity data, most common type
  * **Tags** (`FMassTag`): Marker components with no data
  * **Shared Fragments** (`FMassSharedFragment`): Mutable data shared between entities
  * **Const Shared Fragments** (`FMassConstSharedFragment`): Immutable shared data
  * **Chunk Fragments** (`FMassChunkFragment`): Per-chunk data for entity groups

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
     .NoneFlags({EExampleFlags::Flag4});  // Must not have
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

**Performance Considerations**
Important: The performance difference between `GetMatchingEntities` with a `for` loop and `ForEachEntityChunk` is not as significant as often assumed. For many use cases, especially with smaller entity counts or when you need random access to entities, `GetMatchingEntities` is perfectly acceptable:

```cpp
// This is NOT slow for most gameplay scenarios
TArray<FMassEntityHandle> Enemies = MassAPI.GetMatchingEntities(Query);
for (FMassEntityHandle Enemy : Enemies)
{
    // Process individual entities
    // Good for gameplay logic that needs entity handles
}
```

-----

## C++ API Reference

### UMassAPISubsystem

The main entry point for all Mass API operations.

#### Getting the Subsystem & Core Access

```cpp
// Get as pointer (can be null)
static UMassAPISubsystem* GetPtr(const UObject* WorldContextObject);

// Get as reference (will assert if not available)
static UMassAPISubsystem& GetRef(const UObject* WorldContextObject);

// Get the entity manager
FMassEntityManager* GetEntityManager() const;

// Get the global command buffer
// WARNING: Only use outside of processor contexts!
FORCEINLINE FMassCommandBuffer& Defer() const;
```

#### Query & Filter Operations

```cpp
// Check if archetype composition A contains all of B
FORCEINLINE static bool HasAll(const FMassArchetypeCompositionDescriptor& ThisComposition, const FMassArchetypeCompositionDescriptor& OtherComposition);

// Check if archetype composition A contains any of B
FORCEINLINE static bool HasAny(const FMassArchetypeCompositionDescriptor& ThisComposition, const FMassArchetypeCompositionDescriptor& OtherComposition);

// Check if composition matches ALL requirements of a query
FORCEINLINE static bool MatchQueryAll(const FMassArchetypeCompositionDescriptor& Composition, const FEntityQuery& Query);

// Check if composition matches ANY requirements of a query
FORCEINLINE static bool MatchQueryAny(const FMassArchetypeCompositionDescriptor& Composition, const FEntityQuery& Query);

// Check if composition matches NONE requirements of a query
FORCEINLINE static bool MatchQueryNone(const FMassArchetypeCompositionDescriptor& Composition, const FEntityQuery& Query);

// Check if composition matches all (All, Any, None) requirements of a query
FORCEINLINE static bool MatchQuery(const FMassArchetypeCompositionDescriptor& Composition, const FEntityQuery& Query);

// Check if a specific entity matches all requirements of a query (composition and flags)
FORCEINLINE bool MatchQuery(const FEntityHandle EntityHandle, const FEntityQuery& Query) const;
```

#### Entity Operations (Creation, Destruction)

```cpp
// Check if EntityHandle is valid and built
FORCEINLINE bool IsValid(const FEntityHandle EntityHandle) const;

// Reserve an entity handle without creating its data
FORCEINLINE FMassEntityHandle ReserveEntity() const;

// Build entities from a template (immediate)
TArray<FMassEntityHandle> BuildEntities(int32 Quantity, FMassEntityTemplateData& TemplateData) const;

// Build multiple entities with mixed fragments and tags (immediate)
template<typename... TArgs>
TArray<FMassEntityHandle> BuildEntities(int32 Quantity, TArgs&&... Args) const;

// Build a single entity with mixed fragments and tags (immediate)
template<typename... TArgs>
FORCEINLINE FMassEntityHandle BuildEntity(TArgs&&... Args) const;

// Build a single entity (deferred)
template<typename... TArgs>
FORCEINLINE FMassEntityHandle BuildEntityDefer(FMassCommandBuffer& CommandBuffer, TArgs&&... Args) const;

// Build a single entity (deferred, from context)
template<typename... TArgs>
FORCEINLINE FMassEntityHandle BuildEntityDefer(FMassExecutionContext& Context, TArgs&&... Args) const;

// Build a single entity from template (deferred)
FMassEntityHandle BuildEntityDefer(FMassCommandBuffer& CommandBuffer, FMassEntityTemplateData& TemplateData) const;

// Build a single entity from template (deferred, from context)
FMassEntityHandle BuildEntityDefer(FMassExecutionContext& Context, FMassEntityTemplateData& TemplateData) const;

// Build multiple entities from template (deferred)
void BuildEntitiesDefer(FMassCommandBuffer& CommandBuffer, int32 Quantity, FMassEntityTemplateData& TemplateData, TArray<FMassEntityHandle>& OutEntities) const;

// Build multiple entities from template (deferred, from context)
void BuildEntitiesDefer(FMassExecutionContext& Context, int32 Quantity, FMassEntityTemplateData& TemplateData, TArray<FMassEntityHandle>& OutEntities) const;

// Destroy an entity (immediate)
FORCEINLINE void DestroyEntity(FMassEntityHandle EntityHandle) const;

// Destroy an entity (deferred, from context)
FORCEINLINE void DestroyEntityDefer(FMassExecutionContext& Context, FMassEntityHandle EntityHandle) const;

// Destroy an entity (deferred)
FORCEINLINE void DestroyEntityDefer(FMassCommandBuffer& CommandBuffer, FMassEntityHandle EntityHandle) const;
```

#### Template Data Operations

```cpp
// Get a mutable reference to a fragment within a template
template<typename TFragment>
FORCEINLINE TFragment& GetFragmentRef(FMassEntityTemplateData& TemplateData) const;
```

#### Fragment Operations (Synchronous)

```cpp
// Check if entity has fragment (by type)
FORCEINLINE bool HasFragment(FMassEntityHandle EntityHandle, const UScriptStruct* FragmentType) const;

// Check if entity has fragment (templated)
template<typename T>
FORCEINLINE bool HasFragment(FMassEntityHandle EntityHandle) const;

// Get fragment data by value (copy)
template<typename T>
FORCEINLINE T GetFragment(FMassEntityHandle EntityHandle) const;

// Get fragment data pointer
template<typename T>
FORCEINLINE T* GetFragmentPtr(FMassEntityHandle EntityHandle) const;

// Get fragment data reference
template<typename T>
FORCEINLINE T& GetFragmentRef(FMassEntityHandle EntityHandle) const;

// Add a fragment with value (immediate)
template<typename T>
FORCEINLINE void AddFragment(FMassEntityHandle EntityHandle, const T& FragmentValue) const;

// Add a fragment with FInstancedStruct (immediate)
FORCEINLINE void AddFragment(FMassEntityHandle EntityHandle, const FInstancedStruct& FragmentStruct) const;

// Remove a fragment (templated, immediate)
template<typename T>
FORCEINLINE void RemoveFragment(FMassEntityHandle EntityHandle) const;

// Remove a fragment (by type, immediate)
FORCEINLINE bool RemoveFragment(FMassEntityHandle EntityHandle, UScriptStruct* FragmentType);
```

#### Tag Operations (Synchronous)

```cpp
// Check if entity has tag (by type)
FORCEINLINE bool HasTag(FMassEntityHandle EntityHandle, const UScriptStruct* TagType) const;

// Check if entity has tag (templated)
template<typename T>
FORCEINLINE bool HasTag(FMassEntityHandle EntityHandle) const;

// Add a tag (templated, immediate)
template<typename T>
FORCEINLINE void AddTag(FMassEntityHandle EntityHandle) const;

// Remove a tag (templated, immediate)
template<typename T>
FORCEINLINE void RemoveTag(FMassEntityHandle EntityHandle) const;
```

#### Shared Fragment Operations (Synchronous)

```cpp
// Check if entity has shared fragment (templated)
template<typename T>
FORCEINLINE void HasSharedFragment(FMassEntityHandle EntityHandle) const;

// Get shared fragment data by value (copy)
template<typename T>
FORCEINLINE T GetSharedFragment(FMassEntityHandle EntityHandle) const;

// Get shared fragment data pointer
template<typename T>
FORCEINLINE T* GetSharedFragmentPtr(FMassEntityHandle EntityHandle) const;

// Get shared fragment data reference
template<typename T>
FORCEINLINE T& GetSharedFragmentRef(FMassEntityHandle EntityHandle) const;

// Add a shared fragment (immediate)
template<typename T>
FORCEINLINE bool AddSharedFragment(FMassEntityHandle EntityHandle, const T& SharedFragmentValue) const;

// Remove a shared fragment (templated, immediate)
template<typename T>
FORCEINLINE bool RemoveSharedFragment(FMassEntityHandle EntityHandle) const;

// Remove a shared fragment (by type, immediate)
FORCEINLINE bool RemoveSharedFragment(FMassEntityHandle EntityHandle, const UScriptStruct* SharedFragmentType) const;
```

#### Const Shared Fragment Operations (Synchronous)

```cpp
// Check if entity has const shared fragment (templated)
template<typename T>
FORCEINLINE void HasConstSharedFragment(FMassEntityHandle EntityHandle) const;

// Get const shared fragment data by value (copy)
template<typename T>
FORCEINLINE T GetConstSharedFragment(FMassEntityHandle EntityHandle) const;

// Get const shared fragment data pointer
template<typename T>
FORCEINLINE const T* GetConstSharedFragmentPtr(FMassEntityHandle EntityHandle) const;

// Get const shared fragment data reference
template<typename T>
FORCEINLINE const T& GetConstSharedFragmentRef(FMassEntityHandle EntityHandle) const;

// Add a const shared fragment (immediate)
template<typename T>
FORCEINLINE bool AddConstSharedFragment(FMassEntityHandle EntityHandle, const T& ConstSharedFragmentValue) const;

// Remove a const shared fragment (templated, immediate)
template<typename T>
FORCEINLINE bool RemoveConstSharedFragment(FMassEntityHandle EntityHandle) const;

// Remove a const shared fragment (by type, immediate)
FORCEINLINE bool RemoveConstSharedFragment(FMassEntityHandle EntityHandle, const UScriptStruct* ConstSharedFragmentType) const;
```

#### Flag Operations (Synchronous)

```cpp
// Get all flags as a bitmask
int64 GetEntityFlags(FMassEntityHandle EntityHandle) const;

// Check if entity has a specific flag
bool HasEntityFlag(FMassEntityHandle EntityHandle, EEntityFlags FlagToTest) const;

// Set a flag on an entity
bool SetEntityFlag(FMassEntityHandle EntityHandle, EEntityFlags FlagToSet) const;

// Clear a flag on an entity
bool ClearEntityFlag(FMassEntityHandle EntityHandle, EEntityFlags FlagToClear) const;
```

#### Deferred Operations (with Execution Context)

```cpp
// Deferred add fragment (default value)
template<typename T>
FORCEINLINE void AddFragment(FMassExecutionContext& Context, FMassEntityHandle EntityHandle) const;

// Deferred add fragment (specific value)
template<typename T>
FORCEINLINE void AddFragment(FMassExecutionContext& Context, FMassEntityHandle EntityHandle, const T& FragmentValue) const;

// Deferred remove fragment
template<typename T>
FORCEINLINE void RemoveFragment(FMassExecutionContext& Context, FMassEntityHandle EntityHandle) const;

// Deferred add tag
template<typename T>
FORCEINLINE void AddTag(FMassExecutionContext& Context, FMassEntityHandle EntityHandle) const;

// Deferred remove tag
template<typename T>
FORCEINLINE void RemoveTag(FMassExecutionContext& Context, FMassEntityHandle EntityHandle) const;

// Deferred swap tags
template<typename TOld, typename TNew>
FORCEINLINE void SwapTags(FMassExecutionContext& Context, FMassEntityHandle EntityHandle) const;
```

#### Deferred Operations (with Command Buffer)

```cpp
// Deferred add fragment (default value)
template<typename T>
FORCEINLINE void AddFragment(FMassCommandBuffer& CommandBuffer, FMassEntityHandle EntityHandle) const;

// Deferred add fragment (specific value)
template<typename T>
FORCEINLINE void AddFragment(FMassCommandBuffer& CommandBuffer, FMassEntityHandle EntityHandle, const T& FragmentValue) const;

// Deferred remove fragment
template<typename T>
FORCEINLINE void RemoveFragment(FMassCommandBuffer& CommandBuffer, FMassEntityHandle EntityHandle) const;

// Deferred add tag
template<typename T>
FORCEINLINE void AddTag(FMassCommandBuffer& CommandBuffer, FMassEntityHandle EntityHandle) const;

// Deferred remove tag
template<typename T>
FORCEINLINE void RemoveTag(FMassCommandBuffer& CommandBuffer, FMassEntityHandle EntityHandle) const;

// Deferred swap tags
template<typename TOld, typename TNew>
FORCEINLINE void SwapTags(FMassCommandBuffer& CommandBuffer, FMassEntityHandle EntityHandle) const;
```

-----

## Blueprint API Reference

### Getting Started with Blueprints

The Mass API provides comprehensive Blueprint support through:

  * Custom K2 nodes for type-safe fragment operations
  * Blueprint Function Library with utility functions
  * Blueprint-compatible structs and enums

> **Note:** The Get/Set fragment nodes are provided as custom K2 nodes (see next section) for type safety. The Blueprint Function Library provides the other utility functions.

### Blueprint Function Library

#### Entity Operations (Category: MassAPI|Entity)

| Function | Description | Notes |
| :--- | :--- | :--- |
| **IsValid (EntityHandle)** | Checks if the entity handle is valid and the entity is alive. | Pure function. |
| **Has Mass Fragment** | Checks if an entity has a specific fragment. | Pure function. |
| **Has Mass Tag** | Checks if an entity has a specific tag. | Pure function. |
| **Remove Mass Fragment** | Removes a standard fragment from an entity. | Causes archetype change. |
| **Remove Mass Shared Fragment** | Removes a shared fragment from an entity. | Causes archetype change. |
| **Remove Mass Const Shared Fragment** | Removes a const shared fragment from an entity. | Causes archetype change. |
| **Add Mass Tag** | Adds a tag to an entity. | Causes archetype change. |
| **Remove Mass Tag** | Removes a tag from an entity. | Causes archetype change. |
| **Destroy Entity** | Synchronously destroys a single entity. | |
| **Destroy Entities** | Synchronously destroys multiple entities. | |
| **Build Entity From Template Data** | Synchronously builds a single entity from template data. | |
| **Build Entities From Template Data** | Synchronously builds multiple entities from template data. | |

#### Template Data Operations (Category: MassAPI|Template)

| Function | Description | Notes |
| :--- | :--- | :--- |
| **Get Template Data** | Converts an `FEntityTemplate` (from a data asset) into modifiable `FEntityTemplateData`. | |
| **Template To Template Data** | Auto-cast node to convert `FEntityTemplate` to `FEntityTemplateData`. | Pure, compact node. |
| **IsEmpty (TemplateData)** | Checks if the template data is empty (no fragments or tags). | Pure function. |
| **Has Fragment (TemplateData)** | Checks if the template data contains a specific fragment type. | Pure function. |
| **Has Tag (TemplateData)** | Checks if the template data contains a specific tag type. | Pure function. |
| **Add Tag (TemplateData)** | Adds a tag to the template data. | Modifies template data by-ref. |
| **Remove Tag (TemplateData)** | Removes a tag from the template data. | Modifies template data by-ref. |
| **Remove Fragment in Template** | Removes a fragment from the template data. | Modifies template data by-ref. |
| **Remove Shared Fragment in Template** | Removes a shared fragment from the template data. | Modifies template data by-ref. |
| **Remove Const Shared Fragment in Template** | Removes a const shared fragment from the template data. | Modifies template data by-ref. |

#### Template Flag Operations (Category: MassAPI|Template|Flags)

| Function | Description | Notes |
| :--- | :--- | :--- |
| **Has Template Flag** | Checks if the template data has a specific flag set. | Pure function. |
| **Set Template Flag** | Sets a specific flag in the template data (adds `FEntityFlagFragment` if needed). | Modifies template data by-ref. |
| **Clear Template Flag** | Clears a specific flag in the template data. | Modifies template data by-ref. |

#### Entity Flag Operations (Category: MassAPI|Flags)

| Function | Description | Notes |
| :--- | :--- | :--- |
| **Has Entity Flag** | Checks if an entity has a specific flag set. | Pure function. |
| **Set Entity Flag** | Sets a specific flag on an entity (adds `FEntityFlagFragment` if needed). | |
| **Clear Entity Flag** | Clears a specific flag on an entity. | |

#### Query Operations (Category: MassAPI|Query)

| Function | Description | Notes |
| :--- | :--- | :--- |
| **Match Entity Query** | Checks if a single entity matches all requirements of a query. | Pure function. |
| **Get Matching Entities** | Gets an array of all entity handles that currently match the query. | Warning: Can be slow for large numbers of entities. |

### Blueprint Workflow Examples

**Example 1: Creating and Managing Enemies**

1.  **Create Enemy Template**:
      * Create `FEntityTemplate` variable
      * Add `FEnemyTag` to `Tags` array
      * Add `FHealthFragment` to `Fragments` array
      * Add `FEntityFlagFragment` to `Fragments` array
2.  **Spawn Enemies**:
      * Get Mass API Subsystem
      * Call `Build Entities` with template
      * Store returned entity handles
3.  **Update Enemies**:
      * `For Each Loop` over EntityHandles
      * `Get Mass Fragment` (`FHealthFragment`)
      * Modify health value
      * `Set Mass Fragment` with updated value
4.  **Query Enemies**:
      * Create `FEntityQuery` variable
      * Set `All` list: `$FHealthFragment$, $FEnemyTag$`
      * Set `None` list: `$FDeadTag$`
      * `Get Matching Entities` with query
      * Process results

-----

## K2 Node System

The plugin includes custom K2 nodes for advanced Blueprint integration. These nodes provide type-safe, user-friendly ways to work with Mass entities in Blueprint graphs.

### Custom K2 Nodes

The plugin implements specialized Blueprint nodes that extend the standard function call nodes:

**Base Fragment Function Node (`UK2Node_FragmentFunction`)**
All fragment-related K2 nodes inherit from this base class, which provides:

  * Dynamic pin generation based on selected fragment type
  * Type validation
  * Automatic pin connection management

**Fragment Access Nodes**
| Node Class | Purpose | Special Features |
| :--- | :--- | :--- |
| `UK2Node_GetFragment` | Gets fragment from entity | Dynamically creates output pin matching fragment type |
| `UK2Node_SetFragment` | Sets fragment on entity | Dynamically creates input pin for fragment data |
| `UK2Node_GetSharedFragment` | Gets shared fragment | Handles shared fragment references |
| `UK2Node_SetSharedFragment` | Sets shared fragment | Manages shared instance creation |
| `UK2Node_GetConstSharedFragment` | Gets immutable shared fragment | Read-only access |
| `UK2Node_SetConstSharedFragment` | Sets immutable shared fragment | Creates const shared instances |

**Template Nodes**
| Node Class | Purpose | Special Features |
| :--- | :--- | :--- |
| `UK2Node_GetFragmentFromTemplate` | Extracts fragment from template | Template introspection |
| `UKK2Node_SetFragmentInTemplate` | Adds fragment to template | Template building |
| `UK2Node_SetSharedFragmentInTemplate` | Adds shared fragment to template | Shared instance management |
| `UK2Node_SetConstSharedFragmentInTemplate` | Adds const shared fragment | Const instance management |

### Node Implementation Details

Each K2 node follows this pattern:

  * **Pin Allocation**: Creates pins based on the selected fragment type
  * **Type Validation**: Ensures only valid Mass fragment types are used
  * **Dynamic Updates**: Refreshes pins when fragment type changes
  * **Compilation**: Translates to efficient runtime calls

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

-----

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

  * **Entity Manager Access**: The entity manager is not thread-safe for modifications
  * **Read Operations**: Safe from multiple threads
  * **Write Operations**: Must be synchronized or use command buffers
  * **Deferred Operations**: Thread-safe when each thread uses its own command buffer

> **Important Rule**: Inside processors, always prefer `Context.Defer()` for thread-safe deferred operations. While `MassAPI->AddTag(Context, Entity)` technically works, it's more verbose and less idiomatic. Never use `MassAPI.Defer()` inside processors as it's thread-unsafe.

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

-----

## Best Practices

### Key Guidelines

  * **Context Rules**:
      * **Outside processors**: Use `MassAPI.Defer()`
      * **Inside processors**: Use `Context.Defer()` (best practice)
      * **Avoid**: `MassAPI->AddTag(Context, Entity)` (works but verbose)
  * **Flag System**: Use flags for frequently changing states to avoid archetype migrations
  * **Query Caching**: Build queries once and reuse them
  * **Batch Operations**: Group archetype changes using deferred operations
  * **Entity Validation**: Always check entity handles before use
  * **Fragment Pointers**: Get pointers *after* archetype changes, not before

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

  * **Are you in a Mass Processor?**
      * **YES**
          * **Best Practice** → `Context.Defer().AddTag<>(Entity)`
          * **Alternative** → `MassAPI->AddTag<>(Context, Entity)` [works but verbose]
          * **Never** → `MassAPI->Defer().AddTag<>(Entity)` [thread-unsafe\!]
      * **NO** → Use `MassAPI.Defer()` for deferred operations

-----

## Common Pitfalls

### 1\. Using Wrong Command Buffer Context

  * **Problem**: Using `MassAPI.Defer()` inside processors
    ```cpp
    // ❌ WRONG - Thread-unsafe in processors!
    void Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context)
    {
        MassAPI->Defer().AddTag<FTag>(Entity);
    }
    ```
  * **Solution**: Use `Context.Defer()` in processors
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

### 2\. Fragment Pointer Invalidation

  * **Problem**: Storing fragment pointers across archetype changes
    ```cpp
    // ❌ WRONG
    FHealthFragment* Health = MassAPI.GetFragmentPtr<FHealthFragment>(Entity);
    MassAPI.AddFragment<FShieldFragment>(Entity);  // Archetype change!
    Health->Value = 100.0f;  // Pointer is now invalid!
    ```
  * **Solution**: Get pointers after archetype changes
    ```cpp
    // ✅ CORRECT
    MassAPI.AddFragment<FShieldFragment>(Entity);
    FHealthFragment* Health = MassAPI.GetFragmentPtr<FHealthFragment>(Entity);
    Health->Value = 100.0f;
    ```

### 3\. Forgetting Entity Flag Fragment

  * **Problem**: Trying to use flags without the flag fragment
    ```cpp
    // ❌ WRONG
    FMassEntityHandle Entity = MassAPI.BuildEntity(FEnemyTag{});
    MassAPI.SetEntityFlag(Entity, EEntityFlags::Flag0);  // Crash or undefined behavior!
    ```
  * **Solution**: Always include `FEntityFlagFragment`
    ```cpp
    // ✅ CORRECT
    FMassEntityHandle Entity = MassAPI.BuildEntity(
        FEnemyTag{},
        FEntityFlagFragment{}  // Required for flag operations
    );
    MassAPI.SetEntityFlag(Entity, EEntityFlags::Flag0);
    ```

### 4\. Query System Misunderstanding

  * **Problem**: Expecting `FEntityQuery` to have iteration methods
    ```cpp
    // ❌ WRONG - FEntityQuery doesn't have ForEachEntityChunk
    FEntityQuery Query;
    Query.ForEachEntityChunk(...);  // This doesn't exist!
    ```
  * **Solution**: Use `GetMatchingEntities` for query results
    ```cpp
    // ✅ CORRECT
    FEntityQuery Query;
    TArray<FMassEntityHandle> Results = MassAPI.GetMatchingEntities(Query);
    for (FMassEntityHandle Entity : Results) { /* ... */ }
    ```

### 5\. Performance Anti-Patterns

  * **Problem**: Creating queries every frame
    ```cpp
    // ❌ WRONG - Inefficient
    void Tick()
    {
        FEntityQuery Query;
        Query.All<FHealthFragment>();  // Building query every tick
        auto Results = MassAPI.GetMatchingEntities(Query);
    }
    ```
  * **Solution**: Cache and reuse queries
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

-----

## Conclusion

The Mass API Plugin provides a powerful convenience layer for Unreal Engine's Mass Entity system. The `Defer()` method makes deferred operations intuitive, while maintaining full compatibility with the native Mass Entity API.

Remember the golden rules:

  * Use `MassAPI.Defer()` **outside** processors
  * Use `Context.Defer()` **inside** processors (best practice)
  * While `MassAPI->AddTag(Context, Entity)` works, it's more verbose
  * Include `FEntityFlagFragment` for flag support
  * Cache queries for better performance
  * Validate entity handles before use

This documentation represents the complete state of the Mass API Plugin, providing a robust foundation for building high-performance entity systems while maintaining the ease of use expected in modern Unreal Engine development.

```
```
