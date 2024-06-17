#include "common.hpp"
#include "runtime.hpp"
#include "api/Impl.hpp"
#include "json.hpp"
#include "helpers/v8.hpp"
#include <ghc/filesystem.hpp>
#include <fstream>

bool Runtime::Init(ICore* c, OMPAPI_t* oapi)
{
	core = c;
	ompapi = oapi;

	auto result = node::InitializeOncePerProcess(GetNodeArgs());

	if (result->early_return())
	{
		for (auto& error : result->errors())
		{
			core->logLn(LogLevel::Error, "Error while initializing node: %s", error.c_str());
		}

		return false;
	}

	platform.reset(result->platform());

	auto allocator = node::CreateArrayBufferAllocator();
	isolate = node::NewIsolate(allocator, uv_default_loop(), platform.get());
	node::IsolateData* nodeData = node::CreateIsolateData(isolate, uv_default_loop(), platform.get(), allocator);

	// node::IsolateSettings is;
	// node::SetIsolateUpForNode(isolate, is);

	// IsWorker data slot
	// isolate->SetData(v8::Isolate::GetNumberOfDataSlots() - 1, new bool(false));

	{
		v8::Locker locker(isolate);
		v8::Isolate::Scope isolate_scope(isolate);
		v8::HandleScope handle_scope(isolate);

		context.Reset(isolate, node::NewContext(isolate));
		v8::Context::Scope scope(context.Get(isolate));

		parentEnv = node::CreateEnvironment(nodeData, context.Get(isolate), result->args(), result->exec_args());

		/*
			Load here only needs for debugging as this environment only used as a parent for real environments
		*/

		// node::LoadEnvironment(parentEnv, "console.log('PARENT INIT'); setInterval(() => {}, 1000);");
	}

	EventManager::Instance().Initialize(ompapi);

	return true;
}

void Runtime::RunResources()
{
	auto resourcePaths = GetResourcePathsFromconfig();

	for (auto resourcePath : resourcePaths)
	{
		std::ifstream ifs;
		auto path = ghc::filesystem::canonical("./");
		path /= resourcePath.data();
		path /= "omp-node.json";

		core->logLn(LogLevel::Message, "Loading omp-node resource: %s", path.string().c_str());

		ifs.open(path.string(), std::ifstream::in);

		nlohmann::json resourceConfig;
		try
		{
			resourceConfig = nlohmann::json::parse(ifs, nullptr, true /* allow_exceptions */, true /* ignore_comments */);
		}
		catch (nlohmann::json::exception const& e)
		{
			core->logLn(LogLevel::Error, "Error while parsing resource %s config file: %s", resourcePath.data(), e.what());
		}
		catch (std::ios_base::failure const& e)
		{
			core->logLn(LogLevel::Error, "Error while parsing resource %s config file: %s", resourcePath.data(), e.what());
		}

		if (!(resourceConfig.is_null() || resourceConfig.is_discarded() || !resourceConfig.is_object()))
		{
			ResourceInfo resourceInfo;

			if (resourceConfig["name"].is_null() || resourceConfig["path"].is_null() || resourceConfig["entryFile"].is_null() || resourceConfig["configVersion"].is_null())
			{
				core->logLn(LogLevel::Error, "Error while parsing resource %s config file: Unable to retrieve basic resource config values", resourcePath.data());
			}

			resourceInfo.name = resourceConfig["name"];
			resourceInfo.path = resourceConfig["path"];
			resourceInfo.entryFile = resourceConfig["entryFile"];
			resourceInfo.configVersion = resourceConfig["configVersion"];

			auto resource = CreateImpl(resourceInfo);
			resource->Start();
		}
	}
}

Resource* Runtime::CreateImpl(const ResourceInfo& resource)
{
	auto res = new Resource { this, resource };
	resources.insert(res);
	return res;
}

void Runtime::Tick()
{
	v8::Locker locker(isolate);
	v8::Isolate::Scope isolateScope(isolate);
	v8::HandleScope seal(isolate);
	v8::Context::Scope scope(context.Get(isolate));

	uv_run(uv_default_loop(), UV_RUN_NOWAIT);
	platform->DrainTasks(isolate);
}

void Runtime::Dispose()
{
	{
		v8::SealHandleScope seal(isolate);

		do
		{
			uv_run(uv_default_loop(), UV_RUN_DEFAULT);
			platform->DrainTasks(isolate);
		} while (uv_loop_alive(uv_default_loop()));
	}
	platform->UnregisterIsolate(isolate);
	isolate->Dispose();
	node::FreePlatform(platform.release());
	v8::V8::Dispose();
	v8::V8::ShutdownPlatform();
}

std::vector<Impl::String> Runtime::GetNodeArgs()
{
	// https://nodejs.org/docs/latest-v18.x/api/cli.html#options
	std::vector<Impl::String> args = { "omp-server", "--no-warnings", "--experimental-default-type=module" };

	if (core)
	{
		std::vector<StringView> args(core->getConfig().getStringsCount("node.cli_args"));
		core->getConfig().getStrings("node.cli_args", Span<StringView>(args.data(), args.size()));

		for (auto arg : args)
		{
			args.push_back(arg);
		}
	}

	return args;
}

std::vector<StringView> Runtime::GetResourcePathsFromconfig()
{
	std::vector<StringView> resourcePaths;
	if (core)
	{
		resourcePaths = std::vector<StringView>(core->getConfig().getStringsCount("node.resources"));
		core->getConfig().getStrings("node.resources", Span<StringView>(resourcePaths.data(), resourcePaths.size()));
	}

	return resourcePaths;
}
