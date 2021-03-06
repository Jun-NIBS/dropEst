#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE tests

#include <boost/test/unit_test.hpp>

#include "Estimation/CellsDataContainer.h"

#include <RInside.h>
#include <Estimation/BamProcessing/ReadParamsParser.h>
#include <Estimation/BamProcessing/BamController.h>
#include <Estimation/BamProcessing/BamProcessor.h>
#include <Estimation/Merge/BarcodesParsing/InDropBarcodesParser.h>
#include <Estimation/Merge/BarcodesParsing/ConstLengthBarcodesParser.h>
#include <Estimation/Merge/MergeStrategyFactory.h>
#include <Estimation/Merge/SimpleMergeStrategy.h>
#include <Estimation/Merge/RealBarcodesMergeStrategy.h>
#include <Estimation/Merge/UMIs/MergeUMIsStrategySimple.h>
#include <Estimation/Merge/UMIs/MergeUMIsStrategyDirectional.h>

#include <Tools/Logs.h>
#include <Tools/IndexedValue.h>
#include <Tools/UtilFunctions.h>

using namespace Estimation;
using Mark = UMI::Mark;

static ReadInfo read_info(const std::string &cell_barcode, const std::string &umi, const std::string &gene,
                          const std::string &chr_name = "", const Mark& mark = Mark(Mark::HAS_EXONS))
{
	return ReadInfo(Tools::ReadParameters(cell_barcode, umi, "", umi, 0), gene, chr_name, mark);
}

struct Fixture
{
	Fixture()
		: test_bam_controller(BamProcessing::BamTags(), false, "",
		                      PROJ_DATA_PATH + (std::string)"/gtf/gtf_test.gtf.gz", false, 0)
	{
		auto barcodes_parser = std::shared_ptr<Merge::BarcodesParsing::BarcodesParser>(
				new Merge::BarcodesParsing::InDropBarcodesParser(PROJ_DATA_PATH + std::string("/barcodes/test_est")));

		this->real_cb_strat = std::make_shared<Merge::RealBarcodesMergeStrategy>(barcodes_parser,0, 0, 7, 0);
		this->umi_merge_strat = std::make_shared<Merge::UMIs::MergeUMIsStrategySimple>(1);

		this->any_mark = Mark::get_by_code(Mark::DEFAULT_CODE);
		this->container_full = std::make_shared<CellsDataContainer>(this->real_cb_strat, std::shared_ptr<Merge::UMIs::MergeUMIsStrategyAbstract>(this->umi_merge_strat), this->any_mark);

		Tools::init_test_logs(boost::log::trivial::error);
		this->container_full->add_record(read_info("AAATTAGGTCCA", "AAACCT", "Gene1")); //0, real
		this->container_full->add_record(read_info("AAATTAGGTCCA", "CCCCCT", "Gene2"));
		this->container_full->add_record(read_info("AAATTAGGTCCA", "ACCCCT", "Gene3"));
		this->container_full->add_record(read_info("AAATTAGGTCCA", "ACCCCT", "Gene4"));

		this->container_full->add_record(read_info("AAATTAGGTCCC", "CAACCT", "Gene1")); //1, real
		this->container_full->add_record(read_info("AAATTAGGTCCC", "CAACCT", "Gene10"));
		this->container_full->add_record(read_info("AAATTAGGTCCC", "CAACCT", "Gene20"));

		this->container_full->add_record(read_info("AAATTAGGTCCG", "CAACCT", "Gene1")); //2, false

		this->container_full->add_record(read_info("AAATTAGGTCGG", "AAACCT", "Gene1")); //3, false
		this->container_full->add_record(read_info("AAATTAGGTCGG", "CCCCCT", "Gene2"));

		this->container_full->add_record(read_info("CCCTTAGGTCCA", "CCATTC", "Gene3")); //4, false
		this->container_full->add_record(read_info("CCCTTAGGTCCA", "CCCCCT", "Gene2"));
		this->container_full->add_record(read_info("CCCTTAGGTCCA", "ACCCCT", "Gene3"));

		this->container_full->add_record(read_info("CAATTAGGTCCG", "CAACCT", "Gene1")); //5, false
		this->container_full->add_record(read_info("CAATTAGGTCCG", "AAACCT", "Gene1"));
		this->container_full->add_record(read_info("CAATTAGGTCCG", "CCCCCT", "Gene2"));

		this->container_full->add_record(read_info("AAAAAAAAAAAA", "CCCCCT", "Gene2")); //6, false, excluded
		this->container_full->set_initialized();
	}

	std::shared_ptr<Merge::RealBarcodesMergeStrategy> real_cb_strat;
	std::shared_ptr<Merge::UMIs::MergeUMIsStrategySimple> umi_merge_strat;
	std::shared_ptr<CellsDataContainer> container_full;
	std::vector<Mark> any_mark;
	BamProcessing::BamController test_bam_controller;
};

BOOST_AUTO_TEST_SUITE(TestEstimator)

//	BOOST_FIXTURE_TEST_CASE(testR, Fixture)
//	{
//		using namespace Rcpp;
//		boost::unordered_map<std::string, int> map;
//		map["first"] = 1;
//		map["second"] = 2;
//
//		SEXP res = wrap(map.begin(), map.end());
//
//		RInside R(0, 0);
//		R["saved_vec"] = res;
//		R.parseEvalQ("saveRDS(saved_vec, 'test_rds.rds')");
//	}

	BOOST_FIXTURE_TEST_CASE(testBarcodesFile, Fixture)
	{
		auto cbs = this->real_cb_strat->_barcodes_parser->_barcodes;

		BOOST_CHECK_EQUAL(cbs[0].size(), 3);
		BOOST_CHECK_EQUAL(cbs[1].size(), 3);

		if (cbs[0].size() >= 3)
		{
			BOOST_CHECK_EQUAL(cbs[0][0], "AAT");
			BOOST_CHECK_EQUAL(cbs[0][1], "GAA");
			BOOST_CHECK_EQUAL(cbs[0][2], "AAA");
		}

		if (cbs[1].size() >= 3)
		{
			BOOST_CHECK_EQUAL(cbs[1][0], "TTAGGTCCA");
			BOOST_CHECK_EQUAL(cbs[1][1], "TTAGGGGCC");
			BOOST_CHECK_EQUAL(cbs[1][2], "TTAGGTCCC");
		}

		Merge::BarcodesParsing::InDropBarcodesParser parser("");
		BOOST_CHECK_THROW(parser.get_barcodes_list("/barcodes.wrong"), std::runtime_error);
	}

//	BOOST_FIXTURE_TEST_CASE(testConstLengthBarcodesFile, Fixture)
//	{
//		Merge::BarcodesParsing::ConstLengthBarcodesParser parser(PROJ_DATA_PATH + std::string("/barcodes/hifibio"));
//		parser.init();
//		auto cbs = parser._barcodes;
//
//		BOOST_REQUIRE_EQUAL(cbs.size(), 3);
//
//		BOOST_REQUIRE_EQUAL(cbs[0].size(), 96);
//		BOOST_REQUIRE_EQUAL(cbs[1].size(), cbs[0].size());
//		BOOST_REQUIRE_EQUAL(cbs[2].size(), cbs[0].size());
//
//		BOOST_CHECK_EQUAL(cbs[0][0], "GCTTGGTTGCGCTTGGTTGC");
//		BOOST_CHECK_EQUAL(cbs[1][0], "ACTTGGCGCTACTTGGCGCT");
//		BOOST_CHECK_EQUAL(cbs[2][0], "GCCATTGGACGCCATTGGAC");
//
//		BOOST_CHECK_EQUAL(cbs[0][1], "GGAGTGGTTCGGAGTGGTTC");
//		BOOST_CHECK_EQUAL(cbs[1][1], "AGAACGCTCCAGAACGCTCC");
//		BOOST_CHECK_EQUAL(cbs[2][1], "GCCGAATCCTGCCGAATCCT");
//	}

	BOOST_FIXTURE_TEST_CASE(testIncrement, Fixture)
	{
		std::unordered_map<std::string, int> map;
		auto &t = ++map["chr1"];
		BOOST_CHECK_EQUAL(map["chr1"], 1);
		map.emplace(std::make_pair("chr1", 0));
		BOOST_CHECK_EQUAL(map["chr1"], 1);
		t = 10;
		BOOST_CHECK_EQUAL(map["chr1"], 10);

		auto &t2 = map["chr2"];
		BOOST_CHECK_EQUAL(map["chr2"], 0);
		++t2;
		BOOST_CHECK_EQUAL(map["chr2"], 1);
	}

	BOOST_FIXTURE_TEST_CASE(testUmigsIntersection, Fixture)
	{
		using namespace Merge;
		long is1 = RealBarcodesMergeStrategy::get_umigs_intersect_size(
				this->container_full->cell(this->container_full->cell_id_by_cb("AAATTAGGTCCA")),
				this->container_full->cell(this->container_full->cell_id_by_cb("CCCTTAGGTCCA")));

		long is2 = RealBarcodesMergeStrategy::get_umigs_intersect_size(
				this->container_full->cell(this->container_full->cell_id_by_cb("AAATTAGGTCCC")),
				this->container_full->cell(this->container_full->cell_id_by_cb("AAATTAGGTCCG")));

		long is3 = RealBarcodesMergeStrategy::get_umigs_intersect_size(
				this->container_full->cell(this->container_full->cell_id_by_cb("AAATTAGGTCCA")),
				this->container_full->cell(this->container_full->cell_id_by_cb("AAATTAGGTCCC")));

		BOOST_CHECK_EQUAL(is1, 2);
		BOOST_CHECK_EQUAL(is2, 1);
		BOOST_CHECK_EQUAL(is3, 0);
	}

	BOOST_FIXTURE_TEST_CASE(testFillDistances, Fixture)
	{
		static const std::string ar_cbs1[] = {"AAT", "AAA", "CCT"};
		std::vector<std::string> cbs1(ar_cbs1, ar_cbs1 + sizeof(ar_cbs1) / sizeof(ar_cbs1[0]));

		Merge::BarcodesParsing::InDropBarcodesParser parser("");
		Merge::BarcodesParsing::InDropBarcodesParser::barcode_parts_list_t barcodes;
		barcodes.push_back(cbs1);
		barcodes.push_back(cbs1);

		parser._barcode2_length = 3;
		parser._barcodes = barcodes;
		auto dists = parser.get_distances_to_barcode("ACTACT");

		BOOST_CHECK_EQUAL(dists[0].size(), 3);
		BOOST_CHECK_EQUAL(dists[1].size(), 3);

		BOOST_CHECK_EQUAL(dists[0][0].value, 1);
		BOOST_CHECK_EQUAL(dists[0][1].value, 1);
		BOOST_CHECK_EQUAL(dists[0][2].value, 2);
		BOOST_CHECK_EQUAL(dists[0][2].index, 1);

		BOOST_CHECK_EQUAL(dists[1][0].value, 1);
		BOOST_CHECK_EQUAL(dists[1][1].value, 1);
		BOOST_CHECK_EQUAL(dists[1][2].value, 2);
		BOOST_CHECK_EQUAL(dists[1][2].index, 1);
	}

	BOOST_FIXTURE_TEST_CASE(testRealNeighboursCbs, Fixture)
	{
		CellsDataContainer::ids_t ids = this->real_cb_strat->get_real_neighbour_cbs(*this->container_full, this->container_full->cell_id_by_cb("CAATTAGGTCCG"));

		BOOST_REQUIRE_EQUAL(ids.size(), 2);

		BOOST_CHECK_EQUAL(this->container_full->cell(ids[0]).barcode(), "AAATTAGGTCCA");
		BOOST_CHECK_EQUAL(this->container_full->cell(ids[1]).barcode(), "AAATTAGGTCCC");

		ids = this->real_cb_strat->get_real_neighbour_cbs(*this->container_full, this->container_full->cell_id_by_cb("AAATTAGGTCCC"));

		BOOST_CHECK_EQUAL(ids.size(), 1);

		if (ids.size() >= 2)
		{
			BOOST_CHECK_EQUAL(this->container_full->cell(ids[0]).barcode(), "AAATTAGGTCCC");
		}
	}

	BOOST_FIXTURE_TEST_CASE(testRealNeighbours, Fixture)
	{
		BOOST_CHECK_EQUAL(this->real_cb_strat->get_merge_target(*this->container_full, 0), 0);
		BOOST_CHECK_EQUAL(this->real_cb_strat->get_merge_target(*this->container_full, 1), 1);
		BOOST_CHECK_EQUAL(this->real_cb_strat->get_merge_target(*this->container_full, 2), 1);
		BOOST_CHECK_EQUAL(this->real_cb_strat->get_merge_target(*this->container_full, 3), 0);
		BOOST_CHECK_EQUAL(this->real_cb_strat->get_merge_target(*this->container_full, 4), 0);
		BOOST_CHECK_EQUAL(this->real_cb_strat->get_merge_target(*this->container_full, 5), 0);
	}

	BOOST_FIXTURE_TEST_CASE(testMergeByRealBarcodes, Fixture)
	{
		this->container_full->merge_and_filter();

		BOOST_CHECK_EQUAL(this->container_full->total_cells_number(), 7);
		BOOST_CHECK_EQUAL(this->container_full->filtered_cells().size(), 2);

		BOOST_REQUIRE(this->container_full->filtered_cells().size() >= 2);

		auto &cell0 = this->container_full->cell(this->container_full->filtered_cells()[0]);
		auto &cell1 = this->container_full->cell(this->container_full->filtered_cells()[1]);

		BOOST_CHECK_EQUAL(cell0.size(), 3);
		BOOST_CHECK_EQUAL(cell1.size(), 4);

		BOOST_CHECK_EQUAL(cell0.at("Gene1").size(), 1);
		BOOST_CHECK_EQUAL(cell0.at("Gene1").at("CAACCT").read_count(), 2);

		BOOST_CHECK_EQUAL(cell1.at("Gene1").size(), 2);
		BOOST_CHECK_EQUAL(cell1.at("Gene1").at("AAACCT").read_count(), 3);
		BOOST_CHECK_EQUAL(cell1.at("Gene2").size(), 1);
		BOOST_CHECK_EQUAL(cell1.at("Gene2").at("CCCCCT").read_count(), 4);
		BOOST_CHECK_EQUAL(cell1.at("Gene3").size(), 2);
		BOOST_CHECK_EQUAL(cell1.at("Gene3").at("ACCCCT").read_count(), 2);
		BOOST_CHECK_EQUAL(cell1.at("Gene3").at("CCATTC").read_count(), 1);

		BOOST_CHECK(!this->container_full->cell(0).is_merged());
		BOOST_CHECK(!this->container_full->cell(1).is_merged());
		BOOST_CHECK(this->container_full->cell(2).is_merged());
		BOOST_CHECK(this->container_full->cell(3).is_merged());
		BOOST_CHECK(this->container_full->cell(4).is_merged());
		BOOST_CHECK(this->container_full->cell(5).is_merged());
		BOOST_CHECK(!this->container_full->cell(6).is_merged());

		size_t excluded_num = 0;
		for (size_t i = 0; i < this->container_full->total_cells_number(); ++i)
		{
			if (this->container_full->cell(i).is_excluded())
			{
				excluded_num++;
			}
		}
		BOOST_CHECK_EQUAL(excluded_num, 1);
	}

	BOOST_FIXTURE_TEST_CASE(testSplitBarcode, Fixture)
	{
		Merge::BarcodesParsing::ConstLengthBarcodesParser parser(PROJ_DATA_PATH + std::string("/barcodes/indrop_v3"));
		parser.init();
		auto splitted = parser.split_barcode("TAATGAGCACTAATGA");

		BOOST_REQUIRE_EQUAL(splitted.size(), 2);
		BOOST_CHECK_EQUAL(splitted[0], "TAATGAGC");
		BOOST_CHECK_EQUAL(splitted[1], "ACTAATGA");
	}

	BOOST_FIXTURE_TEST_CASE(testConstLengthBarcodeParser, Fixture)
	{
		Merge::BarcodesParsing::ConstLengthBarcodesParser parser_indrop(PROJ_DATA_PATH + std::string("/barcodes/indrop_v3"));
		parser_indrop.init();

		BOOST_REQUIRE_EQUAL(parser_indrop._barcode_length, 16);
		BOOST_REQUIRE_EQUAL(parser_indrop._barcode_lengths.size(), 2);
		BOOST_CHECK_EQUAL(parser_indrop._barcode_lengths[0], 8);
		BOOST_CHECK_EQUAL(parser_indrop._barcode_lengths[1], 8);
		BOOST_REQUIRE_EQUAL(parser_indrop._barcodes.size(), 2);
		BOOST_CHECK_EQUAL(parser_indrop._barcodes[0].size(), 384);
		BOOST_CHECK_EQUAL(parser_indrop._barcodes[1].size(), 384);

		Merge::BarcodesParsing::ConstLengthBarcodesParser parser_10x(PROJ_DATA_PATH + std::string("/barcodes/10x_aug_2016_split"));
		parser_10x.init();

		BOOST_REQUIRE_EQUAL(parser_indrop._barcode_length, 16);
		BOOST_REQUIRE_EQUAL(parser_10x._barcode_lengths.size(), 2);
		BOOST_CHECK_EQUAL(parser_10x._barcode_lengths[0], 7);
		BOOST_CHECK_EQUAL(parser_10x._barcode_lengths[1], 9);
		BOOST_REQUIRE_EQUAL(parser_10x._barcodes.size(), 2);
		BOOST_CHECK_EQUAL(parser_10x._barcodes[0].size(), 480);
		BOOST_CHECK_EQUAL(parser_10x._barcodes[1].size(), 1536);

		auto distances = parser_10x.get_distances_to_barcode("GGTGCGTAGCTAAACA");
		BOOST_CHECK_EQUAL(distances[0][0].value, 0);
		BOOST_CHECK_EQUAL(distances[1][0].value, 0);
	}

//	BOOST_FIXTURE_TEST_CASE(testStat, Fixture)
//	{
//		Stats stats;
//		stats.set(Stats::StrStrStatType::MERGE_EDIT_DISTANCE_BY_CELL, "AAA", "BBB", 10);
//		stats.set(Stats::StrStrStatType::MERGE_EDIT_DISTANCE_BY_CELL, "CCC", "BBB", 20);
//
//		auto map = stats.get_raw(Stats::StrStrStatType::MERGE_EDIT_DISTANCE_BY_CELL);
//		BOOST_REQUIRE_EQUAL(map.size(), 2);
//
//		BOOST_CHECK_EQUAL(map["AAA"]["BBB"], 10);
//		BOOST_CHECK_EQUAL(map["CCC"]["BBB"], 20);
//	}

	BOOST_FIXTURE_TEST_CASE(testMarkMatch, Fixture)
	{
		using namespace BamProcessing;
		BamTools::BamAlignment align;
		align.Position = 34610;
		align.Length = 10;
		align.CigarData.emplace_back('M', 10);
		ReadParamsParser parser(PROJ_DATA_PATH + (std::string)"/gtf/gtf_test.gtf.gz", BamTags(), false);

		std::string gene;
		Mark umi_mark = parser.get_gene("chrX", align, gene);
		BOOST_CHECK(umi_mark == Mark::HAS_EXONS);

		BOOST_CHECK_EQUAL(gene, "FAM138A");

		align.Position = 34600;
		umi_mark = parser.get_gene("chrX", align, gene);

		BOOST_CHECK(umi_mark.check(Mark::HAS_EXONS));
		BOOST_CHECK(umi_mark.check(Mark::HAS_NOT_ANNOTATED));
		BOOST_CHECK(!umi_mark.check(Mark::HAS_INTRONS));

		BOOST_CHECK_EQUAL(gene, "FAM138A");

		align.Position = 24315;
		umi_mark = parser.get_gene("chr1", align, gene);

		BOOST_CHECK(umi_mark.check(Mark::HAS_EXONS));
		BOOST_CHECK(!umi_mark.check(Mark::HAS_NOT_ANNOTATED));
		BOOST_CHECK(umi_mark.check(Mark::HAS_INTRONS));

		BOOST_CHECK_EQUAL(gene, "WASH7P");
	}

	BOOST_FIXTURE_TEST_CASE(testUmiExclusion, Fixture)
	{
		CellsDataContainer container(this->real_cb_strat, this->umi_merge_strat, Mark::get_by_code("e"));
		container.add_record(read_info("AAATTAGGTCCA", "AAACCT", "Gene1"));
		container.add_record(read_info("AAATTAGGTCCA", "CCCCCT", "Gene2"));
		container.add_record(read_info("AAATTAGGTCCA", "ACCCCT", "Gene3"));
		container.add_record(read_info("AAATTAGGTCCA", "ACCCCT", "Gene4"));

		BOOST_CHECK_EQUAL(container.cell(0).at("Gene4").at("ACCCCT").read_count(), 1);

		container.add_record(read_info("AAATTAGGTCCA", "TTTTTT", "Gene3", "chr1", Mark(Mark::HAS_NOT_ANNOTATED)));
		container.add_record(read_info("AAATTAGGTCCA", "ACCCCT", "Gene4", "chr1", Mark(Mark::HAS_NOT_ANNOTATED)));
		BOOST_CHECK(container.cell(0).at("Gene3").at("TTTTTT").mark().check(Mark::HAS_NOT_ANNOTATED));
		BOOST_CHECK(container.cell(0).at("Gene4").at("ACCCCT").mark().check(Mark::HAS_NOT_ANNOTATED));

		container.set_initialized();
		container.merge_and_filter();

		auto requested_rpus = container.cell(0).requested_reads_per_umi_per_gene(container.gene_match_level());
		BOOST_CHECK_EQUAL(requested_rpus.at("Gene3").at("ACCCCT"), 1);
		BOOST_CHECK_EQUAL(requested_rpus.at("Gene3").size(), 1);

		BOOST_CHECK_THROW(requested_rpus.at("Gene4").at("ACCCCT"), std::out_of_range);
		BOOST_CHECK_THROW(requested_rpus.at("Gene4"), std::out_of_range);
		BOOST_CHECK_EQUAL(container.cell(0).at("Gene4").at("ACCCCT").read_count(), 2);
	}

	BOOST_FIXTURE_TEST_CASE(testGeneMatchLevelUmiExclusion, Fixture)
	{
		using namespace BamProcessing;
		CellsDataContainer container(this->real_cb_strat, this->umi_merge_strat, Mark::get_by_code("e"));
		auto parser = this->test_bam_controller.get_parser();
		std::shared_ptr<BamProcessorAbstract> processor(new BamProcessor(container, BamTags(), false));
		std::unordered_set<std::string> unexpected_chromosomes;

		BamTools::BamAlignment align;
		align.Name = "152228477!TGAGTTCTGTTACTGCATC#ATGGGC";
		align.Position = 34610;
		align.Length = 10;
		align.CigarData.emplace_back('M', 10);

		this->test_bam_controller.process_alignment(parser, processor, unexpected_chromosomes, "chrX", align);
		BOOST_CHECK_EQUAL(container.cell(0).at("FAM138A").at("ATGGGC").read_count(), 1);

		align.Position = 34600;
		this->test_bam_controller.process_alignment(parser, processor, unexpected_chromosomes, "chrX", align);
		BOOST_CHECK(container.cell(0).at("FAM138A").at("ATGGGC").mark().check(Mark::HAS_NOT_ANNOTATED));

		align.Position = 34610;
		this->test_bam_controller.process_alignment(parser, processor, unexpected_chromosomes, "chrX", align);
		BOOST_CHECK(container.cell(0).at("FAM138A").at("ATGGGC").mark().check(Mark::HAS_NOT_ANNOTATED));

		align.Name = "152228477!TGAGTTCTGTTACTGCATC#ATTTTC";
		align.Position = 34600;
		this->test_bam_controller.process_alignment(parser, processor, unexpected_chromosomes, "chrX", align);
		BOOST_CHECK(container.cell(0).at("FAM138A").at("ATTTTC").mark().check(Mark::HAS_NOT_ANNOTATED));

		container.set_initialized();
		container.merge_and_filter();

		BOOST_CHECK_THROW(container.cell(0).requested_reads_per_umi_per_gene(container.gene_match_level()).at("FAM138A"), std::out_of_range);
	}

	BOOST_FIXTURE_TEST_CASE(testGeneMatchLevelUmiExclusion2, Fixture)
	{
		using namespace BamProcessing;
		CellsDataContainer container(this->real_cb_strat, this->umi_merge_strat, Mark::get_by_code("eE"));
		auto parser = this->test_bam_controller.get_parser();
		std::shared_ptr<BamProcessorAbstract> processor(new BamProcessor(container, BamTags(), false));
		std::unordered_set<std::string> unexpected_chromosomes;

		BamTools::BamAlignment align;
		align.Name = "152228477!TGAGTTCTGTTACTGCATC#ATGGGC";
		align.Position = 34610;
		align.Length = 10;
		align.CigarData.emplace_back('M', 10);

		this->test_bam_controller.process_alignment(parser, processor, unexpected_chromosomes, "chrX", align);
		BOOST_CHECK_EQUAL(container.cell(0).at("FAM138A").at("ATGGGC").read_count(), 1);

		align.Position = 34600;
		this->test_bam_controller.process_alignment(parser, processor, unexpected_chromosomes, "chrX", align);
		BOOST_CHECK(container.cell(0).at("FAM138A").at("ATGGGC").mark().check(Mark::HAS_NOT_ANNOTATED));

		align.Name = "152228477!TGAGTTCTGTTACTGCATC#ATTTTC";
		align.Position = 34610;
		this->test_bam_controller.process_alignment(parser, processor, unexpected_chromosomes, "chrX", align);
		BOOST_CHECK(container.cell(0).at("FAM138A").at("ATTTTC").mark()== Mark::HAS_EXONS);

		align.Position = 34600;
		this->test_bam_controller.process_alignment(parser, processor, unexpected_chromosomes, "chrX", align);
		BOOST_CHECK(container.cell(0).at("FAM138A").at("ATTTTC").mark().check(Mark::HAS_NOT_ANNOTATED));

		container.set_initialized();
		container.merge_and_filter();

		BOOST_CHECK_EQUAL(container.cell(0).at("FAM138A").size(), 2);
	}

	BOOST_FIXTURE_TEST_CASE(testUMIMerge, Fixture)
	{
		CellsDataContainer container(this->real_cb_strat, this->umi_merge_strat, this->any_mark);

		container.add_record(read_info("AAATTAGGTCCA", "AAACCT", "Gene1"));
		container.add_record(read_info("AAATTAGGTCCA", "CCCCCT", "Gene1"));
		container.add_record(read_info("AAATTAGGTCCA", "AAATTN", "Gene1"));
		container.add_record(read_info("AAATTAGGTCCA", "ACCCCT", "Gene1"));

		CellsDataContainer::s_s_hash_t merge_targets;
		merge_targets["AAACCT"] = "CCCCCT";
		merge_targets["AAATTN"] = "GGGGGG";
		merge_targets["ACCCCT"] = "ACCCCT";

		container.merge_umis(0, container.gene_indexer().get_index("Gene1"), merge_targets);

		BOOST_REQUIRE_EQUAL(container.cell(0).at("Gene1").size(), 3);
		BOOST_CHECK_EQUAL(container.cell(0).at("Gene1").at("CCCCCT").read_count(), 2);
		BOOST_CHECK_EQUAL(container.cell(0).at("Gene1").at("GGGGGG").read_count(), 1);
		BOOST_CHECK_EQUAL(container.cell(0).at("Gene1").at("ACCCCT").read_count(), 1);
	}

	BOOST_FIXTURE_TEST_CASE(testFillWrongUmis, Fixture)
	{
		Merge::UMIs::MergeUMIsStrategySimple::s_vec_t wrong_umis;
		wrong_umis.emplace_back("AAANTTT");
		wrong_umis.emplace_back("AAANCTT");
		wrong_umis.emplace_back("NNNNNNN");
		auto merge_targets = this->umi_merge_strat->fill_wrong_umis(wrong_umis);
		for (auto const &umi : merge_targets)
		{
			BOOST_CHECK_NE(umi.first, umi.second);
			BOOST_CHECK_EQUAL(Tools::hamming_distance(umi.first, umi.second), 0);
			BOOST_CHECK_EQUAL(umi.second.find('N'), std::string::npos);
		}
	}

	BOOST_FIXTURE_TEST_CASE(testUMIMergeStrategySimple, Fixture)
	{
		CellsDataContainer container(this->real_cb_strat, std::shared_ptr<Merge::UMIs::MergeUMIsStrategyAbstract>(this->umi_merge_strat), this->any_mark);

		container.add_record(read_info("AAATTAGGTCCA", "AAACCT", "Gene1"));
		container.add_record(read_info("AAATTAGGTCCA", "AAACCT", "Gene1"));
		container.add_record(read_info("AAATTAGGTCCA", "AAACCG", "Gene1"));
		container.add_record(read_info("AAATTAGGTCCA", "AAACCN", "Gene1"));
		container.add_record(read_info("AAATTAGGTCCA", "CCCCCT", "Gene1"));
		container.add_record(read_info("AAATTAGGTCCA", "ACCCCT", "Gene1"));

		container.add_record(read_info("AAATTAGGTCCA", "TTTTTT", "Gene2"));
		container.add_record(read_info("AAATTAGGTCCA", "TTTNNG", "Gene2"));
		container.add_record(read_info("AAATTAGGTCCA", "TTGNNG", "Gene2"));
		container.add_record(read_info("AAATTAGGTCCA", "ACCCCT", "Gene2"));
		container.add_record(read_info("AAATTAGGTCCA", "NNNNNN", "Gene2"));

		container.set_initialized();

		this->umi_merge_strat->merge(container);
		BOOST_REQUIRE_EQUAL(container.cell(0).at("Gene1").size(), 4);
		BOOST_REQUIRE_EQUAL(container.cell(0).at("Gene2").size(), 3);

		BOOST_CHECK_EQUAL(container.cell(0).at("Gene1").at("AAACCT").read_count(), 3);
		BOOST_CHECK_EQUAL(container.cell(0).at("Gene1").at("AAACCG").read_count(), 1);
		BOOST_CHECK_EQUAL(container.cell(0).at("Gene1").at("CCCCCT").read_count(), 1);
		BOOST_CHECK_EQUAL(container.cell(0).at("Gene1").at("ACCCCT").read_count(), 1);

		BOOST_CHECK_NO_THROW(container.cell(0).at("Gene2").at("TTTTTT"));
		BOOST_CHECK_NO_THROW(container.cell(0).at("Gene2").at("ACCCCT"));

		for (auto const &umi :container.cell(0).at("Gene2").umis())
		{
			BOOST_CHECK_EQUAL(container.umi_indexer().get_value(umi.first).find('N'), std::string::npos);
		}
	}

	BOOST_FIXTURE_TEST_CASE(testGetGeneWithIntrons, Fixture)
	{
		using namespace BamProcessing;
		auto parser = ReadParamsParser(PROJ_DATA_PATH + (std::string)"/gtf/gtf_test.gtf.gz", BamTags(), false);

		BamTools::BamAlignment align;
		align.Name = "152228477!TGAGTTCTGTTACTGCATC#ATGGGC";
		align.Position = 34610;
		align.Length = 10;
		align.CigarData.emplace_back('M', 10);

		std::string gene;
		auto mark = parser.get_gene("chrX", align, gene);

		BOOST_CHECK(mark == Mark::HAS_EXONS);
		BOOST_CHECK_EQUAL(gene, "FAM138A");

		align.Position = 34600;
		mark = parser.get_gene("chrX", align, gene);
		BOOST_CHECK(mark.check(Mark::HAS_NOT_ANNOTATED));
		BOOST_CHECK(mark.check(Mark::HAS_EXONS));
		BOOST_CHECK(!mark.check(Mark::HAS_INTRONS));
		BOOST_CHECK_EQUAL(gene, "FAM138A");

		align.Position = 23750;
		align.CigarData[0].Length = 1000;
		mark = parser.get_gene("chr1", align, gene);
		BOOST_CHECK(mark.check(Mark::HAS_EXONS));
		BOOST_CHECK(mark.check(Mark::HAS_INTRONS));
		BOOST_CHECK(!mark.check(Mark::HAS_NOT_ANNOTATED));
		BOOST_CHECK_EQUAL(gene, "WASH7P");
	}

	BOOST_FIXTURE_TEST_CASE(testPseudoAlignersGenes, Fixture)
	{
		BamProcessing::BamController controller(BamProcessing::BamTags(), false, "",
												PROJ_DATA_PATH + (std::string)"/gtf/gtf_test.gtf.gz", true, 0);
		auto parser = controller.get_parser();

		BamTools::BamAlignment align;
		std::string chrom_in = "Gene1", gene_out;
		parser->get_gene(chrom_in, align, gene_out);

		BOOST_CHECK_EQUAL(gene_out, chrom_in);
	}

	BOOST_FIXTURE_TEST_CASE(testUMIMergeStrategyDirectional, Fixture)
	{
		using Strat = Merge::UMIs::MergeUMIsStrategyDirectional;
		auto umi_merge_strat = std::make_shared<Strat>();
		CellsDataContainer container(this->real_cb_strat, std::shared_ptr<Merge::UMIs::MergeUMIsStrategyAbstract>(umi_merge_strat), this->any_mark);

		Strat::umi_vec_t umis;
		umis.emplace_back("AAA", 2);
		umis.emplace_back("AAC", 5);
		umis.emplace_back("AAT", 6);
		umis.emplace_back("AGT", 20);
		umis.emplace_back("CCC", 10);
		umis.emplace_back("TCC", 20);

		auto targets = umi_merge_strat->find_targets(umis);

		BOOST_REQUIRE_EQUAL(targets.size(), 3);
		BOOST_CHECK_EQUAL(targets.at("AAA"), "AGT");
		BOOST_CHECK_EQUAL(targets.at("AAT"), "AGT");
		BOOST_CHECK_EQUAL(targets.at("CCC"), "TCC");
	}
BOOST_AUTO_TEST_SUITE_END()
