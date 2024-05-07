using Plots, CSV, DataFrames

function load_data()
    data = CSV.File("../data.csv") |> DataFrame
    s1 = Vector(data[:, 1])
    T1 = Vector(data[:, 2])
    s2 = Vector(data[:, 3])
    T2 = Vector(data[:, 4])
    plot(s1, T1, label="analytic")
    plot!(s2, T2, label="approx")
end
