import pandas as pd

data_folder = "data/"

water_df = pd.read_csv(data_folder + "water.csv")
water_df["label"] = "WATER"

beer_df = pd.read_csv(data_folder + "beer_data_more_liquid.csv")
beer_df["label"] = "BEER"

wine_df = pd.read_csv(data_folder + "red_wine_data.csv")
wine_df["label"] = "WINE"

aperol_df = pd.read_csv(data_folder + "aperol_spritz.csv")
aperol_df["label"] = "APEROL"

rum_df = pd.read_csv(data_folder + "stroh_80.csv")
rum_df["label"] = "RUM"

all_drinks_df = pd.concat(
    [water_df, beer_df, wine_df, aperol_df, rum_df], ignore_index=True
)

all_drinks_df.to_csv(data_folder + "all_drinks_data.csv", index=False)