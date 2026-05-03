<?php

namespace CryptoTrader\Indicators;

/**
 * Technical Indicators Calculator
 */
class Indicators
{
    /**
     * Calculate EMA (Exponential Moving Average)
     */
    public static function calculateEMA($data, $period)
    {
        if (count($data) < $period) {
            throw new \Exception('Insufficient data for EMA calculation');
        }

        $ema = [];
        $multiplier = 2 / ($period + 1);

        // Calculate SMA for first EMA value
        $sma = array_sum(array_slice($data, 0, $period)) / $period;
        $ema[] = $sma;

        // Calculate EMA
        for ($i = $period; $i < count($data); $i++) {
            $emaValue = ($data[$i] - $ema[count($ema) - 1]) * $multiplier + $ema[count($ema) - 1];
            $ema[] = $emaValue;
        }

        return $ema;
    }

    /**
     * Calculate RSI (Relative Strength Index)
     */
    public static function calculateRSI($data, $period = 14)
    {
        if (count($data) < $period + 1) {
            throw new \Exception('Insufficient data for RSI calculation');
        }

        $rsi = [];
        $gains = [];
        $losses = [];

        // Calculate price changes
        for ($i = 1; $i < count($data); $i++) {
            $change = $data[$i] - $data[$i - 1];
            $gains[] = $change > 0 ? $change : 0;
            $losses[] = $change < 0 ? abs($change) : 0;
        }

        // Calculate initial average gain and loss
        $avgGain = array_sum(array_slice($gains, 0, $period)) / $period;
        $avgLoss = array_sum(array_slice($losses, 0, $period)) / $period;

        // Calculate first RSI
        if ($avgLoss == 0) {
            $rsi[] = 100;
        } else {
            $rs = $avgGain / $avgLoss;
            $rsi[] = 100 - (100 / (1 + $rs));
        }

        // Calculate subsequent RSI values
        for ($i = $period; $i < count($gains); $i++) {
            $avgGain = (($avgGain * ($period - 1)) + $gains[$i]) / $period;
            $avgLoss = (($avgLoss * ($period - 1)) + $losses[$i]) / $period;

            if ($avgLoss == 0) {
                $rsi[] = 100;
            } else {
                $rs = $avgGain / $avgLoss;
                $rsi[] = 100 - (100 / (1 + $rs));
            }
        }

        return $rsi;
    }

    /**
     * Calculate MACD (Moving Average Convergence Divergence)
     */
    public static function calculateMACD($data, $fastPeriod = 12, $slowPeriod = 26, $signalPeriod = 9)
    {
        $fastEMA = self::calculateEMA($data, $fastPeriod);
        $slowEMA = self::calculateEMA($data, $slowPeriod);

        // Calculate MACD line
        $macd = [];
        $offset = $slowPeriod - $fastPeriod;
        for ($i = 0; $i < count($slowEMA); $i++) {
            $macd[] = $fastEMA[$i + $offset] - $slowEMA[$i];
        }

        // Calculate signal line
        $signal = self::calculateEMA($macd, $signalPeriod);

        // Calculate histogram
        $histogram = [];
        $signalOffset = count($macd) - count($signal);
        for ($i = 0; $i < count($signal); $i++) {
            $histogram[] = $macd[$i + $signalOffset] - $signal[$i];
        }

        return [
            'macd' => $macd,
            'signal' => $signal,
            'histogram' => $histogram
        ];
    }

    /**
     * Calculate Bollinger Bands
     */
    public static function calculateBollingerBands($data, $period = 20, $stdDev = 2)
    {
        if (count($data) < $period) {
            throw new \Exception('Insufficient data for Bollinger Bands calculation');
        }

        $upper = [];
        $middle = [];
        $lower = [];

        for ($i = $period - 1; $i < count($data); $i++) {
            $slice = array_slice($data, $i - $period + 1, $period);

            // Calculate SMA (middle band)
            $sma = array_sum($slice) / $period;

            // Calculate standard deviation
            $variance = 0;
            foreach ($slice as $value) {
                $variance += pow($value - $sma, 2);
            }
            $std = sqrt($variance / $period);

            $middle[] = $sma;
            $upper[] = $sma + ($stdDev * $std);
            $lower[] = $sma - ($stdDev * $std);
        }

        return [
            'upper' => $upper,
            'middle' => $middle,
            'lower' => $lower
        ];
    }

    /**
     * Calculate ATR (Average True Range)
     */
    public static function calculateATR($high, $low, $close, $period = 14)
    {
        if (count($high) < $period + 1) {
            throw new \Exception('Insufficient data for ATR calculation');
        }

        $trueRanges = [];

        // Calculate True Range
        for ($i = 1; $i < count($close); $i++) {
            $tr1 = $high[$i] - $low[$i];
            $tr2 = abs($high[$i] - $close[$i - 1]);
            $tr3 = abs($low[$i] - $close[$i - 1]);

            $trueRanges[] = max($tr1, $tr2, $tr3);
        }

        // Calculate ATR
        $atr = [];
        $currentATR = array_sum(array_slice($trueRanges, 0, $period)) / $period;
        $atr[] = $currentATR;

        for ($i = $period; $i < count($trueRanges); $i++) {
            $currentATR = (($currentATR * ($period - 1)) + $trueRanges[$i]) / $period;
            $atr[] = $currentATR;
        }

        return $atr;
    }

    /**
     * Calculate SMA (Simple Moving Average)
     */
    public static function calculateSMA($data, $period)
    {
        if (count($data) < $period) {
            throw new \Exception('Insufficient data for SMA calculation');
        }

        $sma = [];

        for ($i = $period - 1; $i < count($data); $i++) {
            $slice = array_slice($data, $i - $period + 1, $period);
            $sma[] = array_sum($slice) / $period;
        }

        return $sma;
    }

    /**
     * Calculate Stochastic Oscillator
     */
    public static function calculateStochastic($high, $low, $close, $period = 14, $smoothK = 3, $smoothD = 3)
    {
        if (count($close) < $period) {
            throw new \Exception('Insufficient data for Stochastic calculation');
        }

        $stochK = [];

        // Calculate %K
        for ($i = $period - 1; $i < count($close); $i++) {
            $highSlice = array_slice($high, $i - $period + 1, $period);
            $lowSlice = array_slice($low, $i - $period + 1, $period);

            $highestHigh = max($highSlice);
            $lowestLow = min($lowSlice);

            if ($highestHigh - $lowestLow == 0) {
                $stochK[] = 50;
            } else {
                $stochK[] = (($close[$i] - $lowestLow) / ($highestHigh - $lowestLow)) * 100;
            }
        }

        // Smooth %K
        $smoothedK = self::calculateSMA($stochK, $smoothK);

        // Calculate %D (SMA of %K)
        $stochD = self::calculateSMA($smoothedK, $smoothD);

        return [
            'k' => $smoothedK,
            'd' => $stochD
        ];
    }
}
