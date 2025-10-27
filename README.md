# **MassAPI Plugin for Unreal Engine 5**

[![Join Discord](https://img.shields.io/badge/Discord-Join%20Chat-blue?logo=discord)](https://discord.gg/KQ59QDbp)
[![QQ Group](https://img.shields.io/badge/QQ%20Group-916358710-blue?logo=tencentqq)](https://jq.qq.com/?_wv=1027&k=5R5X5wX)

**\[UE 5.6.1 compatible\]**

MassAPI is an Unreal Engine 5 plugin designed to significantly simplify interaction with the native Mass Entity Component System (Mass ECS), particularly from Blueprints. It bridges the gap between the high-performance C++ backend of Mass and the user-friendly environment of Blueprints.

## **Overview**

While Mass ECS offers incredible performance for large-scale simulations, its native C++ API is complex and Blueprint accessibility is very limited. MassAPI exposes core Mass functionality through a comprehensive set of Blueprint nodes and C++ utilities, making the system productive for both scripters and programmers.

## **Core Features**

* **Comprehensive Blueprint API:** Exposes entity creation, modification, querying, and template handling via user-friendly Blueprint nodes.  
* **Dynamic Blueprint Nodes:** Uses custom K2Nodes for getting/setting fragments and shared fragments, which dynamically adapt their pins to match your selected struct type.  
* **Blueprint-Safe Data Types:** Wraps native Mass structs (like FMassEntityHandle) into Blueprint-accessible types (FEntityHandle).  
* **Editor-Friendly Templates:** Introduces FEntityTemplate for easily defining entity compositions in Data Assets.  
* **Simplified C++ API:** Provides a centralized UMassAPISubsystem for cleaner and easier C++ interactions.  
* **Blueprint Processor Pattern:** Enables creating Blueprint-based logic loops on entities matching specific queries using the Get Matching Entities node.

## **Quick Start**

1. **Enable Plugin:** Ensure MassAPI and its dependencies (MassEntity, MassGameplay, etc.) are enabled in **Edit \> Plugins**.  
2. **Access Nodes:** Find the plugin's nodes in the Blueprint editor context menu, typically under the **Mass Entity**, **Mass Template**, and **Mass Query** categories.  
3. **C++ Access:** In C++, include MassAPISubsystem.h and get the subsystem:  
   \#include "MassAPISubsystem.h"

   UMassAPISubsystem\* MassAPI \= UMassAPISubsystem::GetPtr(WorldContextObject);  
   if (MassAPI)  
   {  
       // ... use MassAPI-\>...  
   }

## **Core Blueprint Concepts**

MassAPI introduces several key structs for use in Blueprints:

* **FEntityHandle**: A Blueprint-safe wrapper for FMassEntityHandle. This is your reference to a specific entity. Use IsValid (EntityHandle) to check its validity.  
* **FEntityTemplate**: An editor-friendly struct for defining an entity's composition (tags, fragments, shared fragments) in Data Assets.  
* **FEntityTemplateData**: A runtime handle to a template definition. Created from an FEntityTemplate using Get Template Data and can be modified at runtime before spawning.  
* **FEntityQuery**: Defines the criteria for finding entities. Use this to find all entities that have (or do not have) a specific set of components.

## **Key Blueprint Nodes**

### **Entity Operations**

* IsValid (EntityHandle): Checks if an entity handle is still valid.  
* Get Mass Fragment: Retrieves a copy of a fragment's data from an entity.  
* Set Mass Fragment: Sets the value of a fragment on an entity.  
* Add Mass Tag: Adds a tag to an entity.  
* Get Shared Mass Fragment: Retrieves a copy of a shared fragment.  
* Set Shared Mass Fragment: Associates an entity with a shared fragment instance.  
* Destroy Entity: Destroys a single entity.  
* Destroy Entities: Destroys an array of entities in a batch.

### **Entity Building**

* Build Entity From Template Data: Creates a single entity from a template.  
* Build Entities From Template Data: Creates multiple entities from a template in a batch.

### **Template Data Operations**

* Get Template Data: Converts an editor-time FEntityTemplate (from a Data Asset) into a runtime FEntityTemplateData handle.  
* Set Fragment in Template: Sets the initial value for a fragment within the template data.  
* Add Tag (TemplateData): Adds a tag to the template data composition.

### **Entity Querying**

* Match Entity Query: Checks if a single entity matches an FEntityQuery.  
* Get Matching Entities: Returns an array of all active entities that match an FEntityQuery.

**Performance Warning:** Use Get Matching Entities judiciously. Iterating the resulting array in Blueprints to access/modify entity data is **significantly slower** than native C++ Mass Processors, especially for thousands of entities. It is best suited for smaller entity counts or logic that doesn't run every frame.
