#define CATCH_CONFIG_RUNNER
#include <catch.hpp>

#include <LumosEngine.h>

int main(int argc, char * const argv[])
{
	Lumos::Internal::CoreSystem::Init(false);

	int result = Catch::Session().run(argc, argv);

	Lumos::Internal::CoreSystem::Shutdown();

	return result;
}