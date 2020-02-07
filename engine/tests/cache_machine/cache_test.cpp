
#include <cudf/cudf.h>
#include <cudf/types.hpp>
#include <from_cudf/cpp_tests/utilities/base_fixture.hpp>
#include <execution_graph/logic_controllers/LogicPrimitives.h>
#include <src/execution_graph/logic_controllers/LogicalFilter.h>
#include "execution_graph/logic_controllers/LogicalProject.h"
#include "data_builder.h"

struct CacheMachineTest : public cudf::test::BaseFixture {
  CacheMachineTest() {

  }

  ~CacheMachineTest() {
  }

};

TEST_F(CacheMachineTest, CacheMachineTest) {
    unsigned long long  gpuMemory = 1024;
    std::vector<unsigned long long > memoryPerCache = {INT_MAX};
    std::vector<ral::cache::CacheDataType> cachePolicyTypes = {ral::cache::CacheDataType::LOCAL_FILE};
    ral::cache::CacheMachine cacheMachine(gpuMemory, memoryPerCache, cachePolicyTypes);

    for (int i = 0; i < 10; ++i) {
        auto table = build_custom_table();
        std::cout << ">> " << i << "|" <<  table->sizeInBytes() << std::endl;
        cacheMachine.addToCache(std::move(table));
        if (i % 5 == 0) {
            auto cacheTable = cacheMachine.pullFromCache();
        }
    }
    std::this_thread::sleep_for (std::chrono::seconds(1));
}

std::shared_ptr<ral::cache::CacheMachine>  createSourceCacheMachine() {
    unsigned long long  gpuMemory = 1024;
    std::vector<unsigned long long > memoryPerCache = {INT_MAX};
    std::vector<ral::cache::CacheDataType> cachePolicyTypes = {ral::cache::CacheDataType::LOCAL_FILE};
    auto source =  std::make_shared<ral::cache::CacheMachine>(gpuMemory, memoryPerCache, cachePolicyTypes);
    auto table = build_custom_table();
    source->addToCache(std::move(table));
    return source;
}

std::shared_ptr<ral::cache::CacheMachine>  createSinkCacheMachine() {
    unsigned long long  gpuMemory = 1024;
    std::vector<unsigned long long > memoryPerCache = {INT_MAX};
    std::vector<ral::cache::CacheDataType> cachePolicyTypes = {ral::cache::CacheDataType::LOCAL_FILE};
    return std::make_shared<ral::cache::CacheMachine>(gpuMemory, memoryPerCache, cachePolicyTypes);
}


TEST_F(CacheMachineTest, FilterTest){
    using ProcessorFunctor = std::unique_ptr<ral::frame::BlazingTable> (
    const ral::frame::BlazingTableView & table,
    const std::string & query_part,
    blazingdb::manager::experimental::Context * context);

    std::shared_ptr<ral::cache::CacheMachine> cacheSource = createSourceCacheMachine();
    std::shared_ptr<ral::cache::CacheMachine> cacheSink  = createSinkCacheMachine();
    ProcessorFunctor *process_project = &ral::processor::process_filter;
    std::string queryString = "BindableTableScan(table=[[main, nation]], filters=[[<($0, 5)]])";
//    std::string queryString = "BindableTableScan(table=[[main, nation]], projects=[[0, 1]], aliases=[[INT64, INT32]])";
    BlazingContext * context = nullptr;
    int numWorkers = 1;
    ral::cache::ProcessMachine<ProcessorFunctor> processor(cacheSource, cacheSink, process_project, queryString, context, numWorkers);

    std::cout << ">> processor.run()\n";
        processor.run();
    std::cout << "<<> processor.run()\n";
    std::this_thread::sleep_for (std::chrono::seconds(1));
}

TEST_F(CacheMachineTest, ProjectTest) {
    using ProcessorFunctor = std::unique_ptr<ral::frame::BlazingTable> (
            const ral::frame::BlazingTableView & table,
            const std::string & query_part,
            blazingdb::manager::experimental::Context * context);

    std::shared_ptr<ral::cache::CacheMachine> cacheSource = createSourceCacheMachine();
    std::shared_ptr<ral::cache::CacheMachine> cacheSink  = createSinkCacheMachine();
    ProcessorFunctor *process_project = &ral::processor::process_project;
    std::string queryString = "LogicalProject(INT64=[$0], INT32=[$1], FLOAT64=[$2])";
    BlazingContext * context = nullptr;
    int numWorkers = 1;
    ral::cache::ProcessMachine<ProcessorFunctor> processor(cacheSource, cacheSink, process_project, queryString, context, numWorkers);

    std::cout << ">> processor.run()\n";
    processor.run();
    std::cout << "<<> processor.run()\n";
    std::this_thread::sleep_for (std::chrono::seconds(1));
}