#pragma once

#include "BarcodesParser.h"

namespace TestEstimator
{
	struct testBarcodesFile;
	struct testFillDistances;
	struct testRealNeighboursCbs;
	struct testRealNeighbours;
}

namespace Estimation
{
	namespace Merge
	{
		namespace BarcodesParsing
		{
			class InDropBarcodesParser : public BarcodesParser
			{
				friend struct TestEstimator::testBarcodesFile;
				friend struct TestEstimator::testFillDistances;
				friend struct TestEstimator::testRealNeighboursCbs;
				friend struct TestEstimator::testRealNeighbours;

			private:
				size_t _barcode2_length;

			protected:
				barcodes_list_t split_barcode(const std::string &barcode) const override;
				barcode_parts_list_t get_barcodes_list(const std::string &barcodes_filename) const override;

			public:
				explicit InDropBarcodesParser(const std::string &barcodes_filename);
				void init() override;
			};
		}
	}
}