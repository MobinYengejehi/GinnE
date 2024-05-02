/*****************************************************************************
 *
 *  PROJECT:     GninE
 *  LICENSE:     See LICENSE in the top level directory
 *  FILE:        mods/deathmatch/logic/wasm/CWebAssemblyContext.cpp
 *  PURPOSE:     Resource handler class
 *
 *  GninE is available from http://www.gnine.com/
 *
 *****************************************************************************/

#include "StdInc.h"
#include "CLogger.h"

#include "CWebAssemblyContext.h"
#include "CWebAssemblyVariable.h"
#include "CWebAssemblyArgReader.h"

#include "../wasmdefs/CWebAssemblyDefs.h"

CWebAssemblyEngine* WebAssemblyEngine = NULL;

CWebAssemblyEngine::CWebAssemblyEngine()
{
    m_pContext = NULL;
}

CWebAssemblyEngine::CWebAssemblyEngine(const CWebAssemblyEngineContext& context)
{
    SetContext(context);
}

CWebAssemblyEngine::~CWebAssemblyEngine()
{
    Destroy();
}

void CWebAssemblyEngine::Build()
{
    MemAllocOption* options = new MemAllocOption;

    options->allocator.free_func = free;
    options->allocator.malloc_func = malloc;
    options->allocator.realloc_func = realloc;
    
    m_pContext = wasm_engine_new_with_args(Alloc_With_Allocator, options);
}

void CWebAssemblyEngine::Destroy()
{
    if (!m_pContext)
    {
        return;
    }

    wasm_engine_delete(m_pContext);
}

void CWebAssemblyEngine::SetContext(const CWebAssemblyEngineContext& context)
{
    m_pContext = context;
}

CWebAssemblyEngineContext CWebAssemblyEngine::GetContext()
{
    return m_pContext;
}

CWebAssemblyEngine::operator bool()
{
    return m_pContext ? true : false;
}

CWebAssemblyStore::CWebAssemblyStore()
{
    m_pContext = NULL;
}

CWebAssemblyStore::CWebAssemblyStore(const CWebAssemblyStoreContext& context)
{
    SetContext(context);
}

CWebAssemblyStore::CWebAssemblyStore(CWebAssemblyEngine* engine)
{
    SetEngine(engine);
}

CWebAssemblyStore::~CWebAssemblyStore()
{
    Destroy();
}

void CWebAssemblyStore::Build(CWebAssemblyEngine* engine)
{
    SetEngine(engine);

    Build();
}

void CWebAssemblyStore::Build()
{
    if (!m_waEngine)
    {
        return;
    }

    m_pContext = wasm_store_new(m_waEngine->GetContext());
}

void CWebAssemblyStore::Destroy()
{
    if (!m_pContext)
    {
        return;
    }

    wasm_store_delete(m_pContext);

    m_pContext = NULL;
}

void CWebAssemblyStore::SetContext(const CWebAssemblyStoreContext& context)
{
    m_pContext = context;
}

CWebAssemblyStoreContext CWebAssemblyStore::GetContext()
{
    return m_pContext;
}

void CWebAssemblyStore::SetEngine(CWebAssemblyEngine* engine)
{
    m_waEngine = engine;
}

CWebAssemblyEngine* CWebAssemblyStore::GetEngine()
{
    return m_waEngine;
}

CWebAssemblyStore::operator bool()
{
    return m_pContext ? true : false;
}

CWebAssemblyContext::CWebAssemblyContext()
{
    m_pResource = NULL;

    if (WebAssemblyEngine)
    {
        m_pStore = new CWebAssemblyStore(WebAssemblyEngine);
        m_pStore->Build();

        return;
    }

    m_pStore = NULL;
}

CWebAssemblyContext::CWebAssemblyContext(CResource* resource)
{
    SetResource(resource);

    if (WebAssemblyEngine)
    {
        m_pStore = new CWebAssemblyStore(WebAssemblyEngine);
        m_pStore->Build();

        return;
    }

    m_pStore = NULL;
}

CWebAssemblyContext::~CWebAssemblyContext()
{
    Destroy();
}

void CWebAssemblyContext::Destroy()
{
    ClearScripts();
    ClearGlobalFunctions();

    if (m_pStore)
    {
        delete m_pStore;
    }

    m_pStore = NULL;
}

CWebAssemblyScript* CWebAssemblyContext::CreateScript()
{
    return new CWebAssemblyScript(this);
} 

CWebAssemblyLoadState CWebAssemblyContext::LoadScriptBinary(CWebAssemblyScript* script, const char* binary, const size_t& binarySize, const SString& fileName, bool executeMain)
{
    if (!script)
    {
        return CWebAssemblyLoadState::Failed;
    }

    CWebAssemblyLoadState state = script->LoadBinary(binary, binarySize, fileName);

    if (state == CWebAssemblyLoadState::Succeed && executeMain)
    {
        m_lsScripts.push_back(script);

        script->CallMainFunction({ GetResource()->GetName(), fileName, std::to_string(m_lsScripts.size()) });
    }

    return state;
}

CWebAssemblyScriptList& CWebAssemblyContext::GetScripts()
{
    return m_lsScripts;
}

void CWebAssemblyContext::ClearScripts()
{
    if (m_lsScripts.empty())
    {
        return;
    }

    size_t length = m_lsScripts.size();

    for (size_t i = 0; i < length; i++)
    {
        CWebAssemblyScript* script = m_lsScripts[i];

        if (script)
        {
            delete script;
        }
    }

    m_lsScripts.clear();
}

void CWebAssemblyContext::SetResource(CResource* resource)
{
    m_pResource = resource;
}

CResource* CWebAssemblyContext::GetResource()
{
    return m_pResource;
}

CWebAssemblyStore* CWebAssemblyContext::GetStore()
{
    return m_pStore;
}

void CWebAssemblyContext::SetGlobalFunction(const SString& functionName, CWebAssemblyFunction* function)
{
    if (functionName.empty())
    {
        return;
    }

    if (!function)
    {
        return;
    }

    m_mpGlobalFunctions[functionName] = function;
}

CWebAssemblyFunction* CWebAssemblyContext::GetGlobalFunction(const SString& functionName)
{
    if (!DoesGlobalFunctionExist(functionName))
    {
        return NULL;
    }

    return m_mpGlobalFunctions[functionName];
}

CWebAssemblyFunctionMap& CWebAssemblyContext::GetGlobalFunctions()
{
    return m_mpGlobalFunctions;
}

bool CWebAssemblyContext::DoesGlobalFunctionExist(const SString& functionName)
{
    return m_mpGlobalFunctions.find(functionName) != m_mpGlobalFunctions.end();
}

void CWebAssemblyContext::RemoveGlobalFunction(const SString& functionName)
{
    if (!DoesGlobalFunctionExist(functionName))
    {
        return;
    }

    m_mpGlobalFunctions.erase(functionName);
}

void CWebAssemblyContext::ClearGlobalFunctions()
{
    if (m_mpGlobalFunctions.empty())
    {
        return;
    }

    m_mpGlobalFunctions.clear();
}

bool CWebAssemblyContext::DoesPointerBelongToContext(void* ptr)
{
    if (!ptr)
    {
        return false;
    }

    if (m_lsScripts.empty())
    {
        return false;
    }

    for (CWebAssemblyScript* script : m_lsScripts)
    {
        if (script->GetMemory()->DoesPointerBelongToMemory(ptr))
        {
            return true;
        }
    }

    return false;
}

CWebAssemblyScript* CWebAssemblyContext::FindPointerScript(void* ptr)
{
    if (!ptr)
    {
        return NULL;
    }

    if (m_lsScripts.empty())
    {
        return NULL;
    }

    for (CWebAssemblyScript* script : m_lsScripts)
    {
        if (script->GetMemory()->DoesPointerBelongToMemory(ptr))
        {
            return script;
        }
    }

    return NULL;
}

void CWebAssemblyContext::InitializeWebAssemblyEngine()
{
    if (WebAssemblyEngine)
    {
        DeleteWebAssemblyEngine();
    }

    CLogger::LogPrintf("Building new web assembly engine...\n");

    WebAssemblyEngine = new CWebAssemblyEngine();
    WebAssemblyEngine->Build();

    CWebAssemblyDefs::RegisterApiFunctionTypes();

    CLogger::LogPrintf("Web assembly engine built.\n");
}

void CWebAssemblyContext::DeleteWebAssemblyEngine()
{
    if (!WebAssemblyEngine)
    {
        return;
    }

    WebAssemblyEngine->Destroy();

    delete WebAssemblyEngine;

    WebAssemblyEngine = NULL;
}

CWebAssemblyEngine* CWebAssemblyContext::GetWebAssemblyEngine()
{
    return WebAssemblyEngine;
}

bool CWebAssemblyContext::IsWebAssemblyBinary(const char* binary)
{
    if (!binary)
    {
        return false;
    }

    const size_t WASM_BINARY_HEADER_SIZE = 5;
    
	unsigned char wasmHeader[WASM_BINARY_HEADER_SIZE] = {0, 97, 115, 109, 1};

    return std::string(binary, WASM_BINARY_HEADER_SIZE) == std::string((char*)wasmHeader, WASM_BINARY_HEADER_SIZE);
}

CWebAssemblyScript::CWebAssemblyScript()
{
    m_pContextStore = NULL;

    m_pModule = NULL;
    m_pInstance = NULL;

    m_pMemory = NULL;
}

CWebAssemblyScript::CWebAssemblyScript(CWebAssemblyContext* context)
{
    m_pContextStore = context;

    m_pModule = NULL;
    m_pInstance = NULL;

    m_pMemory = NULL;
}

CWebAssemblyScript::~CWebAssemblyScript()
{
    Destroy();
}

void CWebAssemblyScript::CallMainFunction(const std::vector<SString>& argv)
{
    CWebAssemblyFunction* mainFunction = GetExportedFunction(WASM_MAIN_FUNCTION_NAME);

    CWebAssemblyVariables args;
    CWebAssemblyVariables results;

    void*                                         argvs = NULL;
    CWebAssemblyMemoryPointerAddress              argvAddress = WEB_ASSEMBLY_NULL_PTR;
    std::vector<CWebAssemblyMemoryPointerAddress> argvAddressList;

    SString errorMessage;

    if (!mainFunction)
    {
        goto Fail;
    }

    size_t argsLength = argv.size();

    args.Push((int32_t)argsLength);

    if (argsLength > 0)
    {
        argvAddress = m_pMemory->Malloc(argsLength * sizeof(CWebAssemblyMemoryPointerAddress), &argvs);

        CWebAssemblyMemoryPointerAddress* argvList = (CWebAssemblyMemoryPointerAddress*)argvs;

        for (size_t i = 0; i < argsLength; i++)
        {
            argvList[i] = m_pMemory->StringToUTF8(argv[i]);
        }

        args.Push((int32_t)argvAddress);
    }
    else
    {
        args.Push((int32_t)WEB_ASSEMBLY_NULL_PTR);
    }

    if (!mainFunction->Call(&args, &results, &errorMessage))
    {
        goto Fail;
    }

    if (results.GetSize() > 0)
    {
        int32_t exitCode = results.GetFirst().GetInt32();

        if (exitCode)
        {
            CLogger::ErrorPrintf("Web assembly module existed with code : %d\n", exitCode);
        }
    }

    goto CleanUp;

Fail:
    if (!errorMessage.empty())
    {
        CLogger::ErrorPrintf("%s:%s\n", GetResourcePath().c_str(), errorMessage.c_str());

        goto CleanUp;
    }

    if (mainFunction)
    {
        CLogger::ErrorPrintf("Couldn't call web assembly module function '%s'.\n", mainFunction->GetFunctionType().GenerateFunctionStructureText(WASM_MAIN_FUNCTION_NAME).c_str());
        goto CleanUp;
    }

    CLogger::ErrorPrintf("Couldn't call web assembly '%s' function.\n", WASM_MAIN_FUNCTION_NAME);

CleanUp:
    if (argvs)
    {
        CWebAssemblyMemoryPointerAddress* argvList = (CWebAssemblyMemoryPointerAddress*)argvs;

        for (size_t i = 0; i < argsLength; i++)
        {
            m_pMemory->Free(argvList[i]);
        }

        m_pMemory->Free(argvAddress);
    }
}

bool CWebAssemblyScript::CallInternalFunction(const size_t& index, CWebAssemblyVariables* args, CWebAssemblyVariables* results)
{
    CWebAssemblyFunction* function = GetInternalFunction(index);

    if (!function)
    {
        return false;
    }

    return function->Call(args, results);
}

void CWebAssemblyScript::Destroy()
{
    ClearSharedGlobalFunctions();

    ClearExportedFunctions();
    ClearInternalFunctions();
    ClearApiFunctions();
    ClearGlobalFunctions();

    DeleteMemory();

    m_mpExports.clear();

    if (m_pInstance)
    {
        wasm_instance_delete(m_pInstance);
    }

    if (m_pModule)
    {
        wasm_module_delete(m_pModule);
    }
}

CWebAssemblyLoadState CWebAssemblyScript::LoadBinary(const char* binary, const size_t& binarySize, const SString& fileName)
{
    if (binarySize < 1)
    {
        return CWebAssemblyLoadState::Failed;
    }

    if (!CWebAssemblyContext::IsWebAssemblyBinary(binary))
    {
        return CWebAssemblyLoadState::Failed;
    }

    if (!m_pContextStore)
    {
        return CWebAssemblyLoadState::Failed;
    }

    if (m_pInstance || m_pModule)
    {
        Destroy();
    }

    m_strScriptFile = fileName;
    
    CWebAssemblyStoreContext store = m_pContextStore->GetStore()->GetContext();

    if (!store)
    {
        return CWebAssemblyLoadState::Failed;
    }

    std::vector<SString>                   exportNames;
    std::vector<wasm_externkind_t>         exportKinds;
    std::vector<CWebAssemblyExternContext> exports;

    CWebAssemblyExternContext* imports = NULL;
    
    wasm_exporttype_vec_t exportTypes = { 0 };

    wasm_byte_vec_t binaryBuffer;
    binaryBuffer.data = NULL;

    wasm_byte_vec_new_uninitialized(&binaryBuffer, binarySize);

    memcpy(binaryBuffer.data, binary, binarySize);

    if (!wasm_module_validate(store, &binaryBuffer))
    {
        goto Fail;
    }

    m_pModule = wasm_module_new(store, &binaryBuffer);

    wasm_byte_vec_delete(&binaryBuffer);
    binaryBuffer.data = NULL;

    if (!m_pModule)
    {
        goto Fail;
    }

    CWebAssemblyTrap* trap = NULL;

    wasm_importtype_vec_t moduleImportTypes = { 0 };
    wasm_module_imports(m_pModule, &moduleImportTypes);

    size_t moduleImportTypesLength = moduleImportTypes.num_elems;

    if (moduleImportTypesLength > 0)
    {
        imports = new CWebAssemblyExternContext[moduleImportTypesLength];

        for (size_t i = 0; i < moduleImportTypesLength; i++)
        {
            wasm_importtype_t* importType = moduleImportTypes.data[i];

            if (wasm_importtype_is_linked(importType))
            {
                imports[i] = wasm_extern_new_empty(m_pContextStore->GetStore()->GetContext(), wasm_externtype_kind(wasm_importtype_type(importType)));
                continue;
            }

            SString moduleName = wasm_importtype_module(importType)->data;
            SString importName = wasm_importtype_name(importType)->data;

            if (moduleName == WEB_ASSEMBLY_API_MODULE_NAME)
            {
                CWebAssemblyFunctionType apiFunctionType;
                CWebAssemblyFunctionType globalFunctionType;
                CWebAssemblyFunctionType importFuncType;

                wasm_externtype_t* externType = (wasm_externtype_t*)wasm_importtype_type(importType);
                wasm_functype_t*   functype = wasm_externtype_as_functype(externType);

                importFuncType.ReadFunctionTypeContext(functype);

                if (!DoesApiFunctionExist(importName))
                {
                    if (!m_pContextStore->DoesGlobalFunctionExist(importName))
                    {
                        goto ImportFail;
                    }

                    goto CheckGlobalFunctions;
                }

                CWebAssemblyFunction* apiFunction = m_mpApiFunctions[importName];
                
                apiFunctionType = apiFunction->GetFunctionType();

                if (!apiFunctionType.Compare(importFuncType))
                {
                    goto CheckGlobalFunctions;
                }
                
                imports[i] = wasm_func_as_extern(apiFunction->GetFunctionContext());
                
                continue;

            CheckGlobalFunctions:
                if (!m_pContextStore->DoesGlobalFunctionExist(importName))
                {
                    if (apiFunction)
                    {
                        if (!apiFunctionType.Compare(importFuncType))
                        {
                            CLogger::ErrorPrintf("Wrong function structure on import. Defined structure is '%s' [\"@%s/%s\"]\n", apiFunctionType.GenerateFunctionStructureText(importName).c_str(), m_pContextStore->GetResource()->GetName().c_str(), fileName.c_str());
                            goto Fail;
                        }
                    }

                    goto ImportFail;
                }

                //CWebAssemblyFunction* globalFunction = m_mpGlobalFunctions[importName];
                CWebAssemblyFunction* globalFunction = m_pContextStore->GetGlobalFunction(importName);

                globalFunctionType = globalFunction->GetFunctionType();

                if (!globalFunctionType.Compare(importFuncType))
                {
                    CLogger::ErrorPrintf("Wrong function structure on import. Defined structure is '%s' [\"@%s/%s\"]\n", globalFunctionType.GenerateFunctionStructureText(importName).c_str(), m_pContextStore->GetResource()->GetName().c_str(), fileName.c_str());
                    goto Fail;
                }

                RegisterGlobalFunction(importName, globalFunction, false);

                if (!DoesGlobalFunctionExist(importName))
                {
                    goto ImportFail;
                }

                globalFunction = GetGlobalFunction(importName);

                imports[i] = wasm_func_as_extern(globalFunction->GetFunctionContext());

                continue;
                
            ImportFail:
                CWebAssemblyFunction dummyFunction;

                dummyFunction.SetFunctionType(importFuncType);

                RegisterGlobalFunction(importName, &dummyFunction, true);

                if (!DoesGlobalFunctionExist(importName))
                {
                    goto Fail;
                }

                CWebAssemblyFunction* luaFunctionCaller = GetGlobalFunction(importName);

                imports[i] = wasm_func_as_extern(luaFunctionCaller->GetFunctionContext());

                // we are replacing lua functions instead of all not imported functions!
                /*CLogger::ErrorPrintf("Couldn't import function '%s' in script[\"@%s/%s\"].\n", importName.c_str(), m_pContextStore->GetResource()->GetName().c_str(), fileName.c_str());

                imports[i] = wasm_extern_new_empty(m_pContextStore->GetStore()->GetContext(), wasm_externtype_kind(wasm_importtype_type(importType)));

                goto Fail;*/
            }
            else
            {
                imports[i] = wasm_extern_new_empty(m_pContextStore->GetStore()->GetContext(), wasm_externtype_kind(wasm_importtype_type(importType)));                
            }
        }
    }

    wasm_importtype_vec_delete(&moduleImportTypes);
    moduleImportTypes.data = NULL;

    wasm_extern_vec_t moduleImports = { sizeof(imports), imports, moduleImportTypesLength, sizeof(imports) / sizeof(*(imports)), 0 };

    m_pInstance = wasm_instance_new(store, m_pModule, &moduleImports, &trap);

    if (!m_pInstance)
    {
        goto Fail;
    }

    if (imports)
    {
        delete imports;
        imports = NULL;
    }

    wasm_module_exports(m_pModule, &exportTypes);

    size_t length = exportTypes.num_elems;

    if (length > 0 && exportTypes.data)
    {
        for (size_t i = 0; i < length; i++)
        {
            wasm_exporttype_t* exportData = exportTypes.data[i];

            const wasm_name_t* nameData = wasm_exporttype_name(exportData);

            SString name = nameData->data;

            if (!name.empty())
            {
                exportNames.push_back(name);

                const wasm_externtype_t* externType = wasm_exporttype_type(exportData);

                exportKinds.push_back(wasm_externtype_kind(externType));
            }
        }
    }

    wasm_exporttype_vec_delete(&exportTypes);
    exportTypes.data = NULL;

    wasm_extern_vec_t instanceExports = { 0 };
    wasm_instance_exports(m_pInstance, &instanceExports);
    
    length = instanceExports.num_elems;

    if (length > 0 && instanceExports.data)
    {
        for (size_t i = 0; i < length; i++)
        {
            wasm_extern_t* externData = instanceExports.data[i];

            if (externData)
            {
                exports.push_back(externData);
            }
        }
    }

    if (exportNames.size() != exports.size())
    {
        goto Fail;
    }

    length = exports.size();

    for (size_t i = 0; i < length; i++)
    {
        CWebAssemblyExtern externItem;
        externItem.context = exports[i];
        externItem.kind = exportKinds[i];

        m_mpExports[exportNames[i]] = externItem;
    }

    BuildExportedFunctions();
    BuildInternalFunctions();
    BuildMemory();

    InsertSharedGlobalFunctions();

    return CWebAssemblyLoadState::Succeed;

Fail:
    if (exportTypes.data)
    {
        wasm_exporttype_vec_delete(&exportTypes);
    }

    if (moduleImportTypes.data)
    {
        wasm_importtype_vec_delete(&moduleImportTypes);
    }

    if (imports)
    {
        delete imports;
        imports = NULL;
    }

    if (trap)
    {
        wasm_message_t message;
        wasm_trap_message(trap, &message);

        CLogger::ErrorPrintf("Creating new wasm module instance failed : %s\n", message.data);

        wasm_trap_delete(trap);
        wasm_name_delete(&message);
    }

    if (binaryBuffer.data)
    {
        wasm_byte_vec_delete(&binaryBuffer);
    }

    Destroy();

    return CWebAssemblyLoadState::Failed;
}

void CWebAssemblyScript::RegisterApiFunction(const SString& functionName, CWebAssemblyFunctionType functionType, CWebAssemblyCFunction function)
{
    if (functionName.empty())
    {
        return;
    }

    if (DoesApiFunctionExist(functionName))
    {
        DeleteApiFunction(functionName);
    }

    CWebAssemblyFunction* wasmFunction = new CWebAssemblyFunction(m_pContextStore->GetStore(), functionType, function, CreateApiEnvironment(functionName));
    wasmFunction->Build();

    wasmFunction->SetFunctionName(functionName);

    if (!wasmFunction || !wasmFunction->GetFunctionContext())
    {
        return;
    }

    m_mpApiFunctions[functionName] = wasmFunction;
}

void CWebAssemblyScript::RegisterGlobalFunction(const SString& functionName, CWebAssemblyFunction* function, bool isLuaFunction)
{
    if (functionName.empty())
    {
        return;
    }

    if (isLuaFunction)
    {
        if (!function)
        {
            return;
        }

        if (DoesGlobalFunctionExist(functionName))
        {
            DeleteGlobalFunction(functionName);
        }

        CWebAssemblyFunction* wasmFunction = new CWebAssemblyFunction(m_pContextStore->GetStore(), function->GetFunctionType(), Wasm_CallLuaImportedFunction, CreateApiEnvironment(functionName));
        wasmFunction->Build();

        wasmFunction->SetFunctionName(functionName);

        if (!wasmFunction || !wasmFunction->GetFunctionContext())
        {
            return;
        } 

        m_mpGlobalFunctions[functionName] = wasmFunction;

        return;
    }

    if (!function)
    {
        return;
    }

    if (DoesGlobalFunctionExist(functionName))
    {
        DeleteGlobalFunction(functionName);
    }

    CWebAssemblyApiEnviornment sharedEnv = CreateApiEnvironment(functionName);
    
    struct Env
    {
        CWebAssemblyFunction*      function;
        CWebAssemblyApiEnviornment sharedEnv;
    };

    Env* env = new Env();
    env->function = function;
    env->sharedEnv = sharedEnv;

    CWebAssemblyFunction* wasmFunction = new CWebAssemblyFunction(m_pContextStore->GetStore(), function->GetFunctionType(), Wasm_CallSharedImportedFunction, NULL);
    wasmFunction->Build((void*)env);

    wasmFunction->SetFunctionName(functionName);
    wasmFunction->SetApiEnviornment(sharedEnv);

    if (!wasmFunction || !wasmFunction->GetFunctionContext())
    {
        return;
    }

    m_mpGlobalFunctions[functionName] = wasmFunction;
}

WebAssemblyApi(CWebAssemblyScript::Wasm_CallSharedImportedFunction, env, args, results)
{
    struct Env
    {
        CWebAssemblyFunction*      function;
        CWebAssemblyApiEnviornment sharedEnv;
    };

    Env* enviornment = (Env*)env;

    CWebAssemblyFunction* function = enviornment->function;

    CWebAssemblyArgReader argStream(enviornment->sharedEnv, args, results);

    if (!function || !(*function))
    {
        return argStream.ReturnNull("Couldn't call function.\n");
    }
    
    CWebAssemblyVariables funcArgs, funcResults;

    for (size_t i = 0; i < args->num_elems; i++)
    {
        funcArgs.Push(args->data[i]);
    }

    SString errorMessage = "";

    if (!function->Call(&funcArgs, &funcResults, &errorMessage))
    {
        if (!errorMessage.empty())
        {
            //CLogger::ErrorPrintf("%s:%s\n", enviornment->sharedEnv->script->GetResourcePath().c_str(), errorMessage.c_str());

            return argStream.ReturnNull(errorMessage);
        }
    }

    if (funcResults.GetSize() < 1)
    {
        return argStream.ReturnNull();
    }

    return argStream.Return(funcResults.GetFirst());

    return NULL;
}

WebAssemblyApi(CWebAssemblyScript::Wasm_CallLuaImportedFunction, env, args, results)
{
    CLogger::LogPrintf("calling lua shared function in import from wasm!\n");

    return NULL;
}

/*void CWebAssemblyScript::RegisterGlobalFunctions(const SString& functionName, CWebAssemblyFunctionType functionType, CWebAssemblyCFunction function)
{
    if (functionName.empty())
    {
        return;
    }

    if (DoesGlobalFunctionExist(functionName))
    {
        DeleteGlobalFunction(functionName);
    }

    CWebAssemblyFunction* wasmFunction = new CWebAssemblyFunction(m_pContextStore->GetStore(), functionType, function, CreateApiEnvironment(functionName));
    wasmFunction->Build();

    wasmFunction->SetFunctionName(functionName);

    if (!wasmFunction || !wasmFunction->GetFunctionContext())
    {
        return;
    }

    m_mpGlobalFunctions[functionName] = wasmFunction;
}*/ 

CWebAssemblyContext* CWebAssemblyScript::GetStoreContext()
{
    return m_pContextStore;
}

CWebAssemblyModuleContext CWebAssemblyScript::GetModule()
{
    return m_pModule;
}

CWebAssemblyInstanceContext CWebAssemblyScript::GetInstance()
{
    return m_pInstance;
}

void CWebAssemblyScript::BuildExportedFunctions()
{
    if (m_mpExports.empty())
    {
        return;
    }

    for (const std::pair<SString, CWebAssemblyExtern>& item : m_mpExports)
    {
        SString            externName = item.first;
        CWebAssemblyExtern externData = item.second;

        if (externData.kind != C_WASM_EXTERN_TYPE_FUNCTION)
        {
            continue;
        }

        wasm_externtype_t* externType = wasm_extern_type(externData.context);
        wasm_functype_t*   funcType = wasm_externtype_as_functype(externType);

        CWebAssemblyFunctionContext functionContext = wasm_extern_as_func(externData.context);

        CWebAssemblyFunctionType functionType(funcType);

        CWebAssemblyFunction* function = new CWebAssemblyFunction();

        function->SetStore(m_pContextStore->GetStore());
        function->SetFunctionType(functionType);
        function->SetFunctionContext(functionContext);
        function->SetApiEnviornment(CreateApiEnvironment(externName));
        function->SetFunctionName(externName);

        m_mpExportedFunctions[externName] = function;
    }
}

void CWebAssemblyScript::BuildInternalFunctions()
{
    CWebAssemblyExtern ext = GetExport(WASM_INTERNAL_FUNCTIONS_TABLE_EXPORT_NAME);

    if (!IsExternValid(ext))
    {
        return;
    }

    if (ext.kind != C_WASM_EXTERN_TYPE_TABLE)
    {
        return;
    }

    wasm_table_t* functions = wasm_extern_as_table(ext.context);

    if (!functions)
    {
        return;
    }

    size_t count = wasm_table_size(functions);

    if (count < 1)
    {
        return;
    }

    for (size_t i = 0; i < count; i++)
    {
        wasm_ref_t* funcRef = wasm_table_get(functions, i);

        wasm_func_t* wasmFunc = wasm_ref_as_func(funcRef);

        wasm_functype_t* wasmFuncType = wasm_func_type(wasmFunc);

        CWebAssemblyFunctionType funcType;
        funcType.ReadFunctionTypeContext(wasmFuncType);

        CWebAssemblyFunction* internalFunction = new CWebAssemblyFunction();

        internalFunction->SetStore(m_pContextStore->GetStore());
        internalFunction->SetFunctionType(funcType);
        internalFunction->SetFunctionContext(wasmFunc);
        internalFunction->SetApiEnviornment(CreateApiEnvironment(""));

        m_lsInternalFunctions.push_back(internalFunction);
    }
}

void CWebAssemblyScript::BuildMemory()
{
    if (m_mpExports.empty())
    {
        return;
    }

    for (const std::pair<SString, CWebAssemblyExtern>& item : m_mpExports)
    {
        SString            externName = item.first;
        CWebAssemblyExtern externData = item.second;
            
        if (externData.kind != C_WASM_EXTERN_TYPE_MEMORY)
        {
            continue;
        }

        if (m_pMemory)
        {
            DeleteMemory();
        }

        CWebAssemblyMemoryContext memory = wasm_extern_as_memory(externData.context);

        m_pMemory = new CWebAssemblyMemory(this, memory);

        break;
    }
}

CWebAssemblyExternMap& CWebAssemblyScript::GetExports()
{
    return m_mpExports;
}

CWebAssemblyFunctionMap& CWebAssemblyScript::GetExportedFunctions()
{
    return m_mpExportedFunctions;
}

CWebAssemblyExtern CWebAssemblyScript::GetExport(const SString& exportName)
{
    if (!DoesExportExist(exportName))
    {
        CWebAssemblyExtern dummyExtern;
        dummyExtern.context = NULL;
        dummyExtern.kind = NULL;

        return dummyExtern;
    }

    return m_mpExports[exportName];
}

size_t CWebAssemblyScript::GetInternalFunctionsCount()
{
    return m_lsInternalFunctions.size();
}

CWebAssemblyMemory* CWebAssemblyScript::GetMemory()
{
    return m_pMemory;
}

CWebAssemblyFunction* CWebAssemblyScript::GetExportedFunction(const SString& functionName)
{
    if (!DoesExportedFunctionExist(functionName))
    {
        return NULL;
    }

    return m_mpExportedFunctions[functionName];
}

CWebAssemblyFunctionMap& CWebAssemblyScript::GetApiFunctions()
{
    return m_mpApiFunctions;
}

CWebAssemblyFunctionMap& CWebAssemblyScript::GetGlobalFunctions()
{
    return m_mpGlobalFunctions;
}

bool CWebAssemblyScript::DoesExportExist(const SString& exportName)
{
    return m_mpExports.find(exportName) != m_mpExports.end();
}

bool CWebAssemblyScript::DoesExportedFunctionExist(const SString& functionName)
{
    return m_mpExportedFunctions.find(functionName) != m_mpExportedFunctions.end();
}

bool CWebAssemblyScript::DoesApiFunctionExist(const SString& functionName)
{
    return m_mpApiFunctions.find(functionName) != m_mpApiFunctions.end();
}

bool CWebAssemblyScript::DoesGlobalFunctionExist(const SString& functionName)
{
    return m_mpGlobalFunctions.find(functionName) != m_mpGlobalFunctions.end();
}

CWebAssemblyFunction* CWebAssemblyScript::GetApiFunction(const SString& functionName)
{
    if (m_mpApiFunctions.empty())
    {
        return NULL;
    }

    if (!DoesApiFunctionExist(functionName))
    {
        return NULL;
    }

    return m_mpApiFunctions[functionName];
}

CWebAssemblyFunction* CWebAssemblyScript::GetGlobalFunction(const SString& functionName)
{
    if (m_mpGlobalFunctions.empty())
    {
        return NULL;
    }

    if (!DoesGlobalFunctionExist(functionName))
    {
        return NULL;
    }

    return m_mpGlobalFunctions[functionName];
}

CWebAssemblyFunction* CWebAssemblyScript::GetInternalFunction(const size_t& index)
{
    size_t count = m_lsInternalFunctions.size();

    if (count < 1)
    {
        return NULL;
    }

    if (index >= count)
    {
        return NULL;
    }

    return m_lsInternalFunctions[index];
}

void CWebAssemblyScript::DeleteExportedFunction(const SString& functionName)
{
    if (functionName.empty())
    {
        return;
    }

    if (!DoesExportedFunctionExist(functionName))
    {
        return;
    }

    CWebAssemblyFunction* function = m_mpExportedFunctions[functionName];

    if (!function)
    {
        return;
    }

    m_mpExportedFunctions.erase(functionName);

    delete function;
}

void CWebAssemblyScript::DeleteApiFunction(const SString& functionName)
{
    if (functionName.empty())
    {
        return;
    }

    if (!DoesApiFunctionExist(functionName))
    {
        return;
    }

    CWebAssemblyFunction* function = m_mpApiFunctions[functionName];

    if (!function)
    {
        return;
    }

    m_mpApiFunctions.erase(functionName);

    delete function;
}

void CWebAssemblyScript::DeleteGlobalFunction(const SString& functionName)
{
    if (functionName.empty())
    {
        return;
    }

    if (!DoesGlobalFunctionExist(functionName))
    {
        return;
    }

    CWebAssemblyFunction* function = m_mpGlobalFunctions[functionName];

    if (!function)
    {
        return;
    }

    m_mpGlobalFunctions.erase(functionName);

    delete function;
}

void CWebAssemblyScript::ClearExportedFunctions()
{
    if (m_mpExportedFunctions.empty())
    {
        return;
    }

    for (const std::pair<SString, CWebAssemblyFunction*>& item : m_mpExportedFunctions)
    {
        CWebAssemblyFunction* function = item.second;

        if (!function)
        {
            continue;
        }

        delete function;
    }

    m_mpExportedFunctions.clear();
}

void CWebAssemblyScript::ClearInternalFunctions()
{
    if (m_lsInternalFunctions.empty())
    {
        return;
    }

    size_t count = m_lsInternalFunctions.size();

    for (size_t i = 0; i < count; i++)
    {
        CWebAssemblyFunction* function = m_lsInternalFunctions[i];

        if (!function)
        {
            continue;
        }

        delete function;
    }

    m_lsInternalFunctions.clear();
}

void CWebAssemblyScript::ClearApiFunctions()
{
    if (m_mpApiFunctions.empty())
    {
        return;
    }

    for (const std::pair<SString, CWebAssemblyFunction*>& item : m_mpApiFunctions)
    {
        CWebAssemblyFunction* function = item.second;

        if (!function)
        {
            continue;
        }

        delete function;
    }

    m_mpApiFunctions.clear();
}

void CWebAssemblyScript::ClearGlobalFunctions()
{
    if (m_mpGlobalFunctions.empty())
    {
        return;
    }

    for (std::pair<SString, CWebAssemblyFunction*> item : m_mpGlobalFunctions)
    {
        CWebAssemblyFunction* function = item.second;

        if (!function)
        {
            continue;
        }

        delete function;
    }

    m_mpGlobalFunctions.clear();
}

void CWebAssemblyScript::InsertSharedGlobalFunctions()
{
    if (m_mpExportedFunctions.empty())
    {
        return;
    }

    for (const std::pair<SString, CWebAssemblyFunction*>& item : m_mpExportedFunctions)
    {
        SString               name = item.first;
        CWebAssemblyFunction* function = item.second;

        if (name == WASM_MAIN_FUNCTION_NAME || name == WASM_MALLOC_FUNCTION_NAME || name == WASM_FREE_FUNCTION_NAME)
        {
            continue;
        }

        m_pContextStore->SetGlobalFunction(name, function);
    }
}

void CWebAssemblyScript::ClearSharedGlobalFunctions()
{
    CWebAssemblyFunctionMap& globalFunctions = m_pContextStore->GetGlobalFunctions();

    if (globalFunctions.empty())
    {
        return;
    }

    for (std::pair<SString, CWebAssemblyFunction*> item : globalFunctions)
    {
        CWebAssemblyFunction* function = item.second;

        if (function->GetApiEnviornment()->script == this)
        {
            globalFunctions.erase(item.first);
        }
    }
}

void CWebAssemblyScript::DeleteMemory()
{
    if (!m_pMemory)
    {
        return;
    }

    delete m_pMemory;
    m_pMemory = NULL;
}

SString CWebAssemblyScript::GetScriptFile()
{
    return m_strScriptFile;
}

SString CWebAssemblyScript::GetResourcePath()
{
    SString result = m_pContextStore->GetResource()->GetName();
    result += "\\";
    result += m_strScriptFile;

    return result;
}

CWebAssemblyApiEnviornment CWebAssemblyScript::CreateApiEnvironment(const SString& functionName)
{
    CWebAssemblyApiEnviornment env = new CWebAssemblyApiEnvironmentObject();

    env->script = this;
    env->functionName = functionName;

    return env;
}

bool CWebAssemblyScript::IsExternValid(const CWebAssemblyExtern& waExtern)
{
    return waExtern.context != NULL;
}

CWebAssemblyMemory::CWebAssemblyMemory()
{
    m_pContext = NULL;
    m_pScript = NULL;
}

CWebAssemblyMemory::CWebAssemblyMemory(CWebAssemblyScript* script)
{
    SetScript(script);
}

CWebAssemblyMemory::CWebAssemblyMemory(CWebAssemblyMemoryContext context)
{
    SetContext(context);
}

CWebAssemblyMemory::CWebAssemblyMemory(CWebAssemblyScript* script, CWebAssemblyMemoryContext context)
{
    SetScript(script);
    SetContext(context);

    byte_t* base = wasm_memory_data(context);
    size_t size = wasm_memory_data_size(context);
}

CWebAssemblyMemory::~CWebAssemblyMemory()
{
    Destroy();
}

void CWebAssemblyMemory::Destroy()
{
    if (!m_pContext)
    {
        return;
    }

    wasm_memory_delete(m_pContext);
    m_pContext = NULL;
}

CWebAssemblyMemoryPointerAddress CWebAssemblyMemory::Malloc(const size_t& size, void** physicalPointer)
{
    if (size < 1)
    {
        return WEB_ASSEMBLY_NULL_PTR;
    }

    CWebAssemblyFunction* moduleMallocFunction = m_pScript->GetExportedFunction(WASM_MALLOC_FUNCTION_NAME);

    if (!moduleMallocFunction)
    {
        CLogger::ErrorPrintf("Couldn't find module `%s` function[\"@%s/%s\"].\n", WASM_MALLOC_FUNCTION_NAME, m_pScript->GetStoreContext()->GetResource()->GetName().c_str(), m_pScript->GetScriptFile().c_str());
        return WEB_ASSEMBLY_NULL_PTR;
    }

    CWebAssemblyVariables args;
    CWebAssemblyVariables results;

    args.Push((int32_t)size);

    if (!moduleMallocFunction->Call(&args, &results))
    {
        CLogger::ErrorPrintf("Cloudn't allocate memory blocks for module[\"@%s/%s\"].\n", m_pScript->GetStoreContext()->GetResource()->GetName().c_str(), m_pScript->GetScriptFile().c_str());
        return WEB_ASSEMBLY_NULL_PTR;
    }

    if (results.GetSize() < 1)
    {
        return WEB_ASSEMBLY_NULL_PTR;
    }

    CWebAssemblyMemoryPointerAddress pointerAddress = results.GetFirst().GetInt32();

    if (physicalPointer)
    {
        *physicalPointer = GetMemoryPhysicalPointer(pointerAddress);
    }

    return pointerAddress;
}

void CWebAssemblyMemory::Free(CWebAssemblyMemoryPointerAddress pointer)
{
    if (pointer == WEB_ASSEMBLY_NULL_PTR)
    {
        return;
    }
    
    CWebAssemblyFunction* moduleFreeFunction = m_pScript->GetExportedFunction(WASM_FREE_FUNCTION_NAME);

    if (!moduleFreeFunction)
    {
        CLogger::ErrorPrintf("Couldn't find module `%s` function[\"@%s/%s\"].\n", WASM_FREE_FUNCTION_NAME, m_pScript->GetStoreContext()->GetResource()->GetName().c_str(), m_pScript->GetScriptFile().c_str());
        return;
    }

    CWebAssemblyVariables args;
    
    args.Push((int32_t)pointer);

    if (!moduleFreeFunction->Call(&args, NULL))
    {
        CLogger::ErrorPrintf("Cloudn't free memory blocks for module[\"@%s/%s\"].\n", m_pScript->GetStoreContext()->GetResource()->GetName().c_str(), m_pScript->GetScriptFile().c_str());
    }
}

CWebAssemblyMemoryPointerAddress CWebAssemblyMemory::StringToUTF8(const SString& str)
{
    size_t length = str.length();

    if (length < 1)
    {
        return WEB_ASSEMBLY_NULL_PTR;
    }

    size_t size = length + 1;

    void* strPtr = NULL;
    CWebAssemblyMemoryPointerAddress pointer = Malloc(size, &strPtr);

    memcpy(strPtr, (void*)str.data(), length);

    ((char*)strPtr)[length] = '\0';

    return pointer;
}

SString CWebAssemblyMemory::UTF8ToString(CWebAssemblyMemoryPointerAddress pointer, intptr_t size)
{
    SString result = "";

    if (pointer == WEB_ASSEMBLY_NULL_PTR)
    {
        return result;
    }

    char* string = (char*)GetMemoryPhysicalPointer(pointer);

    size_t length = size == -1 ? strlen(string) : size;

    if (length < 1)
    {
        return result;
    }

    result = std::string(string, length);

    return result;
}

void* CWebAssemblyMemory::GetBase()
{
    if (!m_pContext)
    {
        return NULL;
    }

    return wasm_memory_data(m_pContext);
}

size_t CWebAssemblyMemory::GetSize()
{
    if (!m_pContext)
    {
        return 0;
    }

    return wasm_memory_data_size(m_pContext);
}

uintptr_t CWebAssemblyMemory::GetMemoryPhysicalPointerAddress(CWebAssemblyMemoryPointerAddress pointer)
{
    if (pointer == WEB_ASSEMBLY_NULL_PTR)
    {
        return 0;
    }

    return ((uintptr_t)GetBase()) + pointer;
}

void* CWebAssemblyMemory::GetMemoryPhysicalPointer(CWebAssemblyMemoryPointerAddress pointer)
{
    if (pointer == WEB_ASSEMBLY_NULL_PTR)
    {
        return NULL;
    }

    return (void*)((byte_t*)GetBase() + pointer);
}

void CWebAssemblyMemory::SetScript(CWebAssemblyScript* script)
{
    m_pScript = script;
}

CWebAssemblyScript* CWebAssemblyMemory::GetScript()
{
    return m_pScript;
}

void CWebAssemblyMemory::SetContext(CWebAssemblyMemoryContext context)
{
    m_pContext = context;
}

CWebAssemblyMemoryContext CWebAssemblyMemory::GetContext()
{
    return m_pContext;
}

bool CWebAssemblyMemory::DoesPointerBelongToMemory(void* ptr)
{
    if (!ptr)
    {
        return false;
    }

    intptr_t address = (intptr_t)ptr;
    intptr_t base = (intptr_t)GetBase();

    return address >= base && address <= (base + GetSize());
}
