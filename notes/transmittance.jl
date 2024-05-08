using Plots, CSV, DataFrames

function load_data()
    data = CSV.File("../data.csv") |> DataFrame
    s = Vector(data[:, 1])
    T1 = Vector(data[:, 2])
    T2 = Vector(data[:, 3])
    plot(s, T1, label="analytic")
    plot!(s, T2, label="approx")
end
