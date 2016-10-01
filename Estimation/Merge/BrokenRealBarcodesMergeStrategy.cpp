#include "BrokenRealBarcodesMergeStrategy.h"

#include <Tools/UtilFunctions.h>
#include <Tools/Logs.h>

namespace Estimation
{
namespace Merge
{
	BrokenRealBarcodesMergeStrategy::BrokenRealBarcodesMergeStrategy(const std::string &barcodes_filename, size_t barcode2_length,
																	   int min_genes_before_merge, int min_genes_after_merge,
																	   int max_merge_edit_distance, double min_merge_fraction)
		: RealBarcodesMergeStrategy(barcodes_filename, barcode2_length, min_genes_before_merge,
									min_genes_after_merge, max_merge_edit_distance, min_merge_fraction)
	{ }

	long BrokenRealBarcodesMergeStrategy::get_best_merge_target(const CellsDataContainer &container, size_t base_cell_ind,
																 const ul_list_t &neighbour_cells) const
	{
		if (base_cell_ind == neighbour_cells[0])
			return base_cell_ind;

		ul_list_t ids_distr;
		for (auto id : neighbour_cells)
		{
			for (int i = 0; i < (size_t)std::sqrt(container.cell_genes(id).size()) + 1; ++i)
			{
				ids_distr.push_back(id);
			}
		}

		return ids_distr[rand() % ids_distr.size()];
	}

	void BrokenRealBarcodesMergeStrategy::init(const Estimation::CellsDataContainer &container)
	{
		RealBarcodesMergeStrategy::init(container);
		srand(48);
	}

	long BrokenRealBarcodesMergeStrategy::get_max_merge_dist(long min_real_cb_dist) const
	{
		return min_real_cb_dist == 0 ? 0 : min_real_cb_dist + 10;
	}

	std::string BrokenRealBarcodesMergeStrategy::merge_type() const
	{
		return "Broken";
	}
}
}