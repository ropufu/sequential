
if ("R.matlab" %in% rownames(installed.packages()) == FALSE) install.packages("R.matlab");
if ("rstudioapi" %in% rownames(installed.packages()) == FALSE) install.packages("rstudioapi");
rm(list = ls()); # Clear environment.

hist.from.pmf <- function (pmf, count.bins) {
  result <- list();
  class(result) <- "histogram";
  total <- sum(pmf);
  
  result$breaks <- seq(from = 0.5, to = length(pmf) + 0.5, length.out = count.bins + 1);
  result$counts <- integer(count.bins);
  result$density <- numeric(count.bins);
  result$mids <- numeric(count.bins);
  result$xname <- "time";
  result$equidist <- FALSE;
  for (i in 1 : count.bins) {
    index.from <- floor(result$breaks[i] + 1); # Smallest integer strictly > break.
    index.to <- floor(result$breaks[i + 1]); # Largest integer <= break.
    
    result$counts[i] <- sum(pmf[index.from : index.to]);
    result$density[i] <- as.numeric(result$counts[i]) / total;
    result$mids[i] <- (result$breaks[i] + result$breaks[i + 1]) / 2;
  }; # for (...)
}; # hist.from.pmf(...)

setwd(dirname(rstudioapi::getActiveDocumentContext()$path));
data <- R.matlab::readMat('simulator.mat');

thresholds <- data$FMA.W5.thresholds;
pmf <- data$FMA.W5.pmf;

m <- dim(pmf)[1];
n <- dim(pmf)[2];
if (m != length(thresholds)) error("Dimensions mismatch.");

plot(pmf[1, ]);

#unpacked <- rep(0, times = sum(counts));
#offset <- 0;
#for (i in 1 : length(support)) {
#  unpacked[(offset + 1) : (offset + counts[i])] <- support[i];
#  offset <- offset + counts[i];
#} # for (...)

#hist(unpacked, freq = FALSE);
#qqplot(qgeom(ppoints(500), prob = 1 / mean(unpacked)), unpacked)

cat # m=^.^=m~~
