package com.cryptotrader.ml;

import org.springframework.boot.SpringApplication;
import org.springframework.boot.autoconfigure.SpringBootApplication;
import org.springframework.context.annotation.Bean;
import org.springframework.web.servlet.config.annotation.CorsRegistry;
import org.springframework.web.servlet.config.annotation.WebMvcConfigurer;

/**
 * ML Service Application - Main Entry Point
 *
 * Spring Boot application providing machine learning services
 * for cryptocurrency trading signal generation using LSTM and XGBoost.
 */
@SpringBootApplication
public class MLServiceApplication {

    public static void main(String[] args) {
        SpringApplication.run(MLServiceApplication.class, args);
        System.out.println("========================================");
        System.out.println("CryptoTrader ML Service Started");
        System.out.println("API Available at: http://localhost:8080/api/ml");
        System.out.println("Health Check: http://localhost:8080/actuator/health");
        System.out.println("========================================");
    }

    /**
     * Configure CORS to allow requests from PHP backend
     */
    @Bean
    public WebMvcConfigurer corsConfigurer() {
        return new WebMvcConfigurer() {
            @Override
            public void addCorsMappings(CorsRegistry registry) {
                registry.addMapping("/api/**")
                        .allowedOrigins("*")
                        .allowedMethods("GET", "POST", "PUT", "DELETE", "OPTIONS")
                        .allowedHeaders("*")
                        .maxAge(3600);
            }
        };
    }
}
